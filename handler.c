/* handler.c: HTTP Request Handlers */

#include "spidey.h"

#include <errno.h>
#include <limits.h>
#include <string.h>

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

/* Internal Declarations */
HTTPStatus handle_browse_request(Request *request);
HTTPStatus handle_file_request(Request *request);
HTTPStatus handle_cgi_request(Request *request);
HTTPStatus handle_error(Request *request, HTTPStatus status);

/**
 * Handle HTTP Request.
 *
 * @param   r           HTTP Request structure
 * @return  Status of the HTTP request.
 *
 * This parses a request, determines the request path, determines the request
 * type, and then dispatches to the appropriate handler type.
 *
 * On error, handle_error should be used with an appropriate HTTP status code.
 **/
HTTPStatus  handle_request(Request *r) {
    HTTPStatus result = HTTP_STATUS_OK;

    /* Parse request */
    if (parse_request(r) == -1)
    {
        fprintf(stderr, "parse_request failed: %s\n", strerror(errno));
        result = HTTP_STATUS_BAD_REQUEST;
        result = handle_error(r, result);
        return result;
    }

    /* Determine request path */
    r->path = determine_request_path(r->uri); 
    if (r->path == NULL)
    {
        fprintf(stderr, "determine_request_path failed: %s\n", strerror(errno));
        result = HTTP_STATUS_NOT_FOUND;
        result = handle_error(r, result);
        return result;
    }
    debug("HTTP REQUEST PATH: %s", r->path);

    /* Dispatch to appropriate request handler type based on file type */
    struct stat s;
    if(stat(r->path, &s) < 0)
    {
        fprintf(stderr, "stat failed: %s\n", strerror(errno));
        result = HTTP_STATUS_INTERNAL_SERVER_ERROR;
        return result;
    }
    int cgi = access(r->path, X_OK);
    int file_num = access(r->path, R_OK);

    if ((s.st_mode & S_IFMT) == S_IFDIR)
    {
        result = handle_browse_request(r);
    }
    /*if (cgi < 0)
    {
        fprintf(stderr, "access failed: %s\n", strerror(errno));
        result = HTTP_STATUS_INTERNAL_SERVER_ERROR;
    }*/
    if (cgi == 0 && (s.st_mode & S_IFMT) == S_IFREG) 
    {
        result = handle_cgi_request(r);
    }

    /*if (file_num < 0)
    {   
        fprintf(stderr, "access failed: %s\n", strerror(errno)); 
        result = HTTP_STATUS_INTERNAL_SERVER_ERROR;
    }*/
    else if (file_num == 0 && cgi != 0)
    {
        debug("got in here");
        result = handle_file_request(r);
    }

    log("HTTP REQUEST STATUS: %s", http_status_string(result));
    return result;
}

/**
 * Handle browse request.
 *
 * @param   r           HTTP Request structure.
 * @return  Status of the HTTP browse request.
 *
 * This lists the contents of a directory in HTML.
 *
 * If the path cannot be opened or scanned as a directory, then handle error
 * with HTTP_STATUS_NOT_FOUND.
 **/
HTTPStatus  handle_browse_request(Request *r) {
    struct dirent **entries;
    int n;

    /* Open a directory for reading or scanning */
    n = scandir(r->path, &entries, NULL, alphasort);
    if (n < 0)
    {
        fprintf(stderr, "scandir failed: %s\n", strerror(errno));
        free(entries);
        return HTTP_STATUS_NOT_FOUND;
    }

    /* Write HTTP Header with OK Status and text/html Content-Type */
    fprintf(r->file, "HTTP/1.0 200 OK\n");
    fprintf(r->file, "Content-Type: text/html\n");
    fprintf(r->file, "\r\n");


    /* For each entry in directory, emit HTML list item */
    fprintf(r->file, "<ul>");
    for (int i = 0; i < n; i++)
    {
        if(streq(entries[i]->d_name,".")){
            free(entries[i]);
            continue;
        }
        if(streq(r->uri,"/")){
            fprintf(r->file, "<li><a href=\"%s%s\">%s</a></li>\n",r->uri,entries[i]->d_name, entries[i]->d_name);
        } else {
            fprintf(r->file, "<li><a href=\"%s/%s\">%s</a></li>\n",r->uri,entries[i]->d_name, entries[i]->d_name);
        }
        free(entries[i]);
    }
    fprintf(r->file, "</ul>");

    /* Flush socket, return OK */
    free(entries);
    if (fflush(r->file) < 0)
    {
        fprintf(stderr, "fflush failed: %s\n", strerror(errno));
        return 418;
    }
    return HTTP_STATUS_OK;
}

/**
 * Handle file request.
 *
 * @param   r           HTTP Request structure.
 * @return  Status of the HTTP file request.
 *
 * This opens and streams the contents of the specified file to the socket.
 *
 * If the path cannot be opened for reading, then handle error with
 * HTTP_STATUS_NOT_FOUND.
 **/
HTTPStatus  handle_file_request(Request *r) {
    FILE *fs;
    char buffer[BUFSIZ];
    char *mimetype = NULL;
    //size_t nread;

    /* Open file for reading */
    fs = fopen(r->path, "r");
    if (!fs)
    {
        fprintf(stderr, "fopen failed: %s\n", strerror(errno));
        goto fail;
    }

    /* Determine mimetype */
    mimetype = determine_mimetype(r->path);

    /* Write HTTP Headers with OK status and determined Content-Type */
    fprintf(r->file, "HTTP/1.0 200 OK\n");
    fprintf(r->file, "Content-Type: %s\n", mimetype);
    fprintf(r->file, "\r\n");

    /* Read from file and write to socket in chunks */
    int nread;
    while((nread = fread(buffer, 1, BUFSIZ, fs)) > 0)
    {
        int nwritten = fwrite(buffer,1,nread, r->file);
        while(nwritten != nread){
            nwritten += fwrite(buffer+nwritten,1,nread-nwritten,r->file);
        }
    }

    /* Close file, flush socket, deallocate mimetype, return OK */
    fclose(fs);
    if (fflush(r->file) < 0)
    {
        fprintf(stderr, "fflush failed: %s\n", strerror(errno));
        return 418;
    }
    free(mimetype);
    return HTTP_STATUS_OK;

fail:
    /* Close file, free mimetype, return INTERNAL_SERVER_ERROR */
    free(mimetype);
    return HTTP_STATUS_INTERNAL_SERVER_ERROR;
}

/**
 * Handle CGI request
 *
 * @param   r           HTTP Request structure.
 * @return  Status of the HTTP file request.
 *
 * This popens and streams the results of the specified executables to the
 * socket.
 *
 * If the path cannot be popened, then handle error with
 * HTTP_STATUS_INTERNAL_SERVER_ERROR.
 **/
HTTPStatus handle_cgi_request(Request *r) {
    FILE *pfs;
    char buffer[BUFSIZ];

    /* Export CGI environment variables from request structure:
     * http://en.wikipedia.org/wiki/Common_Gateway_Interface */
    if(!r->query){
        setenv("QUERY_STRING","",1);
    } else {
        setenv("QUERY_STRING",r->query,1);
    }
    setenv("REMOTE_PORT",r->port,1);
    setenv("REQUEST_METHOD",r->method,1);
    setenv("REQUEST_URI",r->uri,1);
    setenv("REMOTE_ADDR",r->host,1);
    setenv("DOCUMENT_ROOT",RootPath,1);
    setenv("SCRIPT_FILENAME",r->path,1);
    setenv("SERVER_PORT",Port,1);
 
    //DOCUMENT_ROOT, SCRIPT_FILENAME, SERVER_PORT
    /* Export CGI environment variables from request headers */
    struct header *headerptr = r->headers;
    while(headerptr)
    {
        if(streq(headerptr->name,"Host")){
            setenv("HTTP_HOST",headerptr->value,1);
        }
        else if(streq(headerptr->name,"Accept")){
            setenv("HTTP_ACCEPT",headerptr->value,1);
        }
        else if(streq(headerptr->name,"Accept-Language")){
            setenv("HTTP_ACCEPT_LANGUAGE",headerptr->value,1);
        }
        else if(streq(headerptr->name,"Accept-Encoding")){
            setenv("HTTP_ACCEPT_ENCODING",headerptr->value,1);
        }
        else if(streq(headerptr->name,"Connection")){
            setenv("HTTP_CONNECTION",headerptr->value,1);
        }
        else if(streq(headerptr->name,"User-Agent")){
            setenv("HTTP_USER_AGENT",headerptr->value,1);
        }
        headerptr = headerptr->next;
    }

    /* POpen CGI Script */
    debug("r->path: %s",r->path);
    pfs = popen(r->path, "r"); // pass in path user requested 
    if (pfs == NULL)
    {
        fprintf(stderr, "popen failed: %s\n", strerror(errno));
        pclose(pfs);
        return HTTP_STATUS_NOT_FOUND;
    }

    /* Copy data from popen to socket */
    while(fgets(buffer, BUFSIZ, pfs))
    {
        fputs(buffer, r->file);
    }

    /* Close popen, flush socket, return OK */
    pclose(pfs);
    if (fflush(r->file) < 0)
    {
        fprintf(stderr, "fflush failed: %s\n", strerror(errno));
        return 418;
    }
    return HTTP_STATUS_OK;
}

/**
 * Handle displaying error page
 *
 * @param   r           HTTP Request structure.
 * @return  Status of the HTTP error request.
 *
 * This writes an HTTP status error code and then generates an HTML message to
 * notify the user of the error.
 **/
HTTPStatus  handle_error(Request *r, HTTPStatus status) {
    debug("got into handle_error");
    const char *status_string = http_status_string(status);
    // 200
    // 400 - bad request
    // 404 - not found
    // 500 - internal server error

    /* Write HTTP Header */
    fprintf(r->file, "HTTP/1.0 %s\r\n",status_string);
    fprintf(r->file, "Content-Type: text/html\r\n");
    fprintf(r->file, "\r\n");

    /* Write HTML Description of Error*/
    fprintf(r->file, "<li> %s</li>\n",status_string);

    fflush(r->file);
    /* Return specified status */
    return status;
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
