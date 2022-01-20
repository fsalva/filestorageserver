#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include <errno.h>
#include <signal.h>

#include<pthread.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/time.h>

#include "libs/config_parser.h"
#include "libs/server.h"
#include "libs/icl_hash.h"

char * s_n;

void intHandler() {
    fflush(stdout);
    unlink(s_n);
    exit(0);
}


int main(int argc, char const *argv[])
{   
    config_parser *     cp;
    icl_hash_t *        hashtable;
    
    if(argc > 0) {if(argv[0] == NULL) {} }  //--    ignorami :) 
    
    

    cp = malloc(sizeof(config_parser));

    parse(cp);    
    
    int file_server;
    if((file_server = start_server(cp->thread_workers_n, cp->max_dim, cp->max_file_n, cp->socket_path)) == 1){}
    
    s_n = malloc(sizeof(cp->socket_path));
    strcpy(s_n, cp->socket_path);

    fprintf(stderr, "\n\n%s\n\n", s_n);

    signal(SIGINT, intHandler);
    signal(SIGPIPE, SIG_IGN);

    hashtable = icl_hash_create(8, hash_pjw, string_compare);

    icl_hash_insert(hashtable, "shit", "fuck");
    icl_hash_insert(hashtable, "shit", "test");
    icl_hash_insert(hashtable, "test", "test");
    icl_hash_insert(hashtable, "test", "fuck");

    icl_hash_dump(stderr, hashtable);
    void * x = icl_hash_find(hashtable, "test");
    void * y = icl_hash_find(hashtable, "shit");
    void * z = icl_hash_find(hashtable, "fuck");

    
    fprintf(stderr, "%s %s %s", (char *) x, (char *) y, (char *) z);

    loop_server();

    config_cleanup(cp);
    
    return 0;
}
