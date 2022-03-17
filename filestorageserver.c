#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include <pthread.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/time.h>

#include "libs/config_parser.h"
#include "libs/server.h"
#include "libs/icl_hash.h"

char * s_n;

void intHandler() {
    fprintf(stdout, " \n\n HANDLED %d connections!!! ", handledSuccessfully);
    fflush(stdout);
    unlink(s_n);
    exit(0);
}


int main(int argc, char const *argv[])
{   
    config_parser *     cp;    
    if(argc > 0) {if(argv[0] == NULL) {} }  //--    ignorami :) 

    cp = malloc(sizeof(config_parser));

    parse(cp);    
    
    int file_server;
    if((file_server = start_server(cp->thread_workers_n, cp->max_dim, cp->max_file_n, cp->socket_path)) == 1){}

    s_n = malloc(sizeof(char) * strlen(cp->socket_path) + 1);
    strncpy(s_n, cp->socket_path, strlen(cp->socket_path));

    signal(SIGINT, intHandler);
    signal(SIGPIPE, SIG_IGN);
    

    loop_server();

    config_cleanup(cp);

    free(s_n);
    
    return 0;
}
