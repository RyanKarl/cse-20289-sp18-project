#!/usr/bin/env python3

import multiprocessing
import os
import requests
import sys
import time

# Globals

PROCESSES = 1
REQUESTS  = 1
VERBOSE   = False
URL       = None

# Functions

def usage(status=0):
    print('''Usage: {} [-p PROCESSES -r REQUESTS -v] URL
    -h              Display help message
    -v              Display verbose output

    -p  PROCESSES   Number of processes to utilize (1)
    -r  REQUESTS    Number of requests per process (1)
    '''.format(os.path.basename(sys.argv[0])))
    sys.exit(status)

def do_request(pid):
    ''' Perform REQUESTS HTTP requests and return the average elapsed time. '''
    elapsedTime = 0;
    for request in range(REQUESTS): 
        start = time.time()
        r = requests.get(URL)
        end = time.time()
        if r.status_code != 200:
            print("Error. Problem with request")
        if VERBOSE:
            print(r.text)
        print("Process: {}, Request: {}, Elapsed Time: {:.2f}".format(pid,request,end-start))
        elapsedTime += end-start

    print("Process: {}, AVERAGE   , Elapsed Time: {:.2f}".format(pid,elapsedTime/REQUESTS))
   

    return elapsedTime/REQUESTS

# Main execution

if __name__ == '__main__':
    # Parse command line arguments
    args = sys.argv[1:]
    while args and args[0].startswith('-') and len(args[0]) > 1:
        arg = args.pop(0)
        if arg == "-v":
            VERBOSE = True
        elif arg == "-p":
            PROCESSES = int(args.pop(0))
        elif arg == "-r":
            REQUESTS = int(args.pop(0))
        elif arg == "-h":
            usage(0)

    if len(args) == 1:
        URL = args.pop()
    else:
        usage(1)



    print(PROCESSES, REQUESTS, VERBOSE, URL) 

    

        ##error("client: socket");


        # Allow for multiprocessing based on number of cores specified
    CORES = multiprocessing.cpu_count()
    if PROCESSES < CORES:
        CORES = PROCESSES 
    pool = multiprocessing.Pool(CORES)
    timeList = pool.map(do_request,range(PROCESSES))
    totalTime = 0
    for num in timeList:
       totalTime += num 
    print("TOTAL AVERAGE ELAPSED TIME: {:.2f}".format(totalTime/PROCESSES))



    # Create pool of workers and perform requests
    pass

# vim: set sts=4 sw=4 ts=8 expandtab ft=python:
