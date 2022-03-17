#ifndef _MY_FILE_H_
#define _MY_FILE_H_

#include "./list.h"

typedef struct _myfile
{
    char *      filename;
    void *      content;
    elem_t *    fd_list;
    int         flags;
} myfile;


#endif
