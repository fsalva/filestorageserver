#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include <errno.h>

#include<pthread.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/time.h>

#include "libs/config_parser.h"
#include "libs/server.h"


int main(int argc, char const *argv[])
{   
    if(argc > 0) {if(argv[0] == NULL) {} }  //--    ignorami :) 

    config_parser * cp;
    
    cp = malloc(sizeof(config_parser));

    parse(cp);    
    
    int file_server;
    if((file_server = start_server(cp->thread_workers_n, cp->max_dim, cp->max_file_n, cp->socket_path)) == 1){}
    
    loop_server();
    
    config_cleanup(cp);
    
    return 0;
}
