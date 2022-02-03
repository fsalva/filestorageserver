#ifndef SUPPORTED_OPERATIONS_H
#define SUPPORTED_OPERATIONS_H

#include "./icl_hash.h"

int read_file();
int read_n_files();

size_t write_file(char *, int, int , icl_hash_t *);
int write_n_files();


#endif