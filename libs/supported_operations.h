#ifndef SUPPORTED_OPERATIONS_H
#define SUPPORTED_OPERATIONS_H

#include "./icl_hash.h"

int 
read_file(), 
    read_n_files();

size_t 
write_file(char *, int, int , icl_hash_t *, int);

int 
write_n_files();

int 
create_file(char * path, int c_pid, int c_socket, icl_hash_t *, int flag);

int 
lock_file(char * path, int c_pid, int c_socket, icl_hash_t * hashtable, int flag);

void * 
get_file(int);

size_t
get_file_size(FILE * );



#endif