#ifndef STRING_H_UTILS_FILESTORAGE
#define STRING_H_UTILS_FILESTORAGE

#include<stdio.h>
#include<string.h>
#include<ctype.h>

#include "../stringutils.h"

void trim(char * s) {
    char * p = s;
    int l = strlen(p);

    while(isspace(p[l - 1])) p[--l] = 0;
    while(* p && isspace(* p)) ++p, --l;

    memmove(s, p, l + 1);
} 

#endif