#ifndef FILESTORAGE_REQUEST_H
#define FILESTORAGE_REQUEST_H

typedef struct _request
{
    long    r_op_code;
    long    r_pid;
    char *  r_body;

} request;

request * parse_request(char *);

#endif