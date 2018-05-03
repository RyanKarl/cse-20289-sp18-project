/* request.c: HTTP Request Functions */

#include "spidey.h"

#include <errno.h>
#include <string.h>

#include <unistd.h>

int parse_request_method(Request *r);
int parse_request_headers(Request *r);


/**
 * Accept request from server socket.
 *
 * @param   sfd         Server socket file descriptor.
 * @return  Newly allocated Request structure.
 *
 * This function does the following:
 *
 *  1. Allocates a request struct initialized to 0.
 *  2. Initializes the headers list in the request struct.
 *  3. Accepts a client connection from the server socket.
 *  4. Looks up the client information and stores it in the request struct.
 *  5. Opens the client socket stream for the request struct.
 *  6. Returns the request struct.
 *
 * The returned request struct must be deallocated using free_request.
 **/
Request * accept_request(int sfd) {
    Request *r;
    struct sockaddr raddr;
    socklen_t rlen = sizeof(struct sockaddr);

    /* Allocate request struct (zeroed) */
    r = calloc(1, sizeof(Request));
    if (r == NULL)
    {
        fprintf(stderr, "calloc failed: %s\n", strerror(errno));
        goto fail;
    }
    
    /* Accept a client */
    int client_fd;

    if ((client_fd = accept(sfd, &raddr, &rlen)) < 0) {
        fprintf(stderr, "accept failed: %s\n", strerror(errno));
        goto fail;
    }
    r->fd = client_fd;

    /* Lookup client information */
    int  flags = NI_NUMERICHOST | NI_NUMERICSERV;
    int  status;
    if ((status = getnameinfo(&raddr, rlen, r->host, sizeof(r->host), r->port, sizeof(r->port), flags)) != 0) {
        fprintf(stderr, "Unable to lookup request : %s\n", gai_strerror(status));
        goto fail;
    }

    r->headers = NULL;

    /* Open socket stream */
    FILE *client_file = fdopen(client_fd, "w+");
    if (!client_file) {
        fprintf(stderr, "fdopen failed: %s\n", strerror(errno));
        goto fail;
    }
    r->file = client_file;

    log("Accepted request from %s:%s", r->host, r->port);
    return r;

fail:
    /* Deallocate request struct */
    free_request(r);
    return NULL;
}

/**
 * Deallocate request struct.
 *
 * @param   r           Request structure.
 *
 * This function does the following:
 *
 *  1. Closes the request socket stream or file descriptor.
 *  2. Frees all allocated strings in request struct.
 *  3. Frees all of the headers (including any allocated fields).
 *  4. Frees request struct.
 **/
void free_request(Request *r) {
    if (!r) {
    	return;
    }

    /* Close socket or fd */
    if(r->file){
        fclose(r->file);
    } else {
        close(r->fd);
    }

    /* Free allocated strings */
    free(r->method);
    free(r->uri);
    free(r->path);
    free(r->query);

    /* Free headers */
    Header * h = r->headers;
    Header * temp = r->headers;
    while (h != NULL)
    {
        temp = h;
        h = h->next;
        free(temp->name);
        free(temp->value);
        free(temp);
    }
    r->headers = NULL;

    /* Free request */
    free(r);
    r = NULL;
}
/**
 * Parse HTTP Request.
 *
 * @param   r           Request structure.
 * @return  -1 on error and 0 on success.
 *
 * This function first parses the request method, any query, and then the
 * headers, returning 0 on success, and -1 on error.
 **/
int parse_request(Request *r) {
    /* Parse HTTP Request Method */
    int prm = parse_request_method(r);
    if (prm == -1)
    {
        return -1;
    }

    /* Parse HTTP Requet Headers*/
    int prh = parse_request_headers(r); 
    if (prh == -1)
    {
        return -1;
    }

    return 0;
}

/**
 * Parse HTTP Request Method and URI.
 *
 * @param   r           Request structure.
 * @return  -1 on error and 0 on success.
 *
 * HTTP Requests come in the form
 *
 *  <METHOD> <URI>[QUERY] HTTP/<VERSION>
 *
 * Examples:
 *
 *  GET / HTTP/1.1
 *  GET /cgi.script?q=foo HTTP/1.0
 *
 * This function extracts the method, uri, and query (if it exists).
 **/
int parse_request_method(Request *r) {
    char buffer[BUFSIZ];
    char *method;
    char *uri;
    char *query;
    char *replace;

    /* Read line from socket */
    if ((fgets(buffer, BUFSIZ, r->file)) < 0) 
    {
        fprintf(stderr, "fgets failed: %s\n", strerror(errno));
        goto fail;
    }

    /* Parse method and uri */
    method = strtok(buffer, " ");
    if (method == NULL)
    {
        fprintf(stderr, "strtok failed: %s\n", strerror(errno));
        goto fail;
    }

    uri = strtok(NULL, " ");
    debug("uri: %s",uri);
    if (uri == NULL)
    {
        fprintf(stderr, "strtok failed: %s\n", strerror(errno));
        goto fail;
    }
    
    /* Parse query from uri */
    replace = strchr(uri,'?');
    query = strchr(uri, '?');
    if(query){
        *replace = '\0';
        query++;
    }
    debug("query: %s:", query);

    /* Record method, uri, and query in request struct */
    r->method = strdup(method);
    if (r->method == NULL)
    {
        fprintf(stderr, "strdup failed: %s\n", strerror(errno));
        free(r->method);
        goto fail;
    }
    r->uri = strdup(uri);
    if (r->uri == NULL)
    {
        fprintf(stderr, "strdup failed: %s\n", strerror(errno));
        free(r->uri);
        goto fail;
    }
    if(query){
        r->query = strdup(query);
        if (r->query == NULL)
        {
            fprintf(stderr, "strdup failed: %s\n", strerror(errno));
            free(r);
            goto fail;
        }
    } else {
        r->query = NULL;
    }

    debug("HTTP METHOD: %s", r->method);
    debug("HTTP URI:    %s", r->uri);
    debug("HTTP QUERY:  %s", r->query);

    return 0;

fail:
    return -1;
}

/**
 * Parse HTTP Request Headers.
 *
 * @param   r           Request structure.
 * @return  -1 on error and 0 on success.
 *
 * HTTP Headers come in the form:
 *
 *  <NAME>: <VALUE>
 *
 * Example:
 *
 *  Host: localhost:8888
 *  User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:29.0) Gecko/20100101 Firefox/29.0
 *  Accept: text/html,application/xhtml+xml
 *  Accept-Language: en-US,en;q=0.5
 *  Accept-Encoding: gzip, deflate
 *  Connection: keep-alive
 *
 * This function parses the stream from the request socket using the following
 * pseudo-code:
 *
 *  while (buffer = read_from_socket() and buffer is not empty):
 *      name, value = buffer.split(':')
 *      header      = new Header(name, value)
 *      headers.append(header)
 **/
int parse_request_headers(Request *r) {

    char buffer[BUFSIZ];
    char *name;
    char *value;
    struct header *header;

    /* Parse headers from socket */
    while ((fgets(buffer, BUFSIZ, r->file)) != NULL && !streq(buffer,"\r\n"))
    {
        debug("buffer: %s",buffer);
        name = buffer;
        name = skip_whitespace(name);
        name = strtok(name,":");
        if(!name){
            goto fail;
        }
        debug("Name: %s",name);
        value = strtok(NULL,"\r");
        if(!value){
            goto fail;
        }
        value = skip_whitespace(value);
        debug("value: %s",value);
        header = malloc(sizeof(Header));
        header->name = strdup(name);        
        header->value = strdup(value);
        header->next = r->headers;
        r->headers = header;
    }
    

#ifndef NDEBUG
    for (struct header *header = r->headers; header != NULL; header = header->next) {
    	debug("HTTP HEADER %s = %s", header->name, header->value);
    }

#endif
    return 0;

fail:
    return -1;
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
