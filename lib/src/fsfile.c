#include <stdlib.h>
#include "../fslist.h"

typedef struct _file_t {

    char * _pathname;
    void * _content;
    size_t _f_size;
    elem_t * _open_fd;
    

} file_t;