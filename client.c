#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <limits.h>

#define BUF_SIZE 4096

int openConnection(const char * sockname, int msec, const struct timespec abst);
int closeConnection(const char * sockname);

int main(int argc, char const *argv[])
{
    //Nome socket.
    char * socketName = "./socket/l.sock";
    char buf[BUF_SIZE];

    int fd_skt, fd_c; 
    struct sockaddr_un sa;

    strncpy(sa.sun_path, socketName, 108);
    sa.sun_family=AF_UNIX;

    fd_skt = socket(AF_UNIX,SOCK_STREAM,0);
    if(connect(fd_skt, (struct sockaddr *) &sa, sizeof(sa)) == -1){
        if(errno == ENOENT)
            sleep(1);
        else
            exit(EXIT_FAILURE);
    }


    char * req  = "/tmp/TESTFILES/lipsum15.txt\n";
    char * killmsg = "\r\n\r\n";

    char * req2 = "/tmp/TESTFILES/lipsum1.txt\n";

int letti = 0;
    memset(buf, 0, BUF_SIZE);


    //manda richiesta
    write(fd_skt, req, strlen(req));

    size_t bytes_read;
    size_t dataLen = 0;
        
    while((bytes_read = (recv(fd_skt, buf, sizeof(buf), 0))) > 0){

        dataLen += bytes_read;

        if( buf[dataLen] == '\000' || dataLen > (BUF_SIZE-1)) break;
    }

    letti += dataLen;

    write(fd_skt, req2, strlen(req2));
    //write(fd_skt, req, strlen(killmsg));

     while((bytes_read = (recv(fd_skt, buf, sizeof(buf), 0))) > 0){

        dataLen += bytes_read;

        if( buf[dataLen] == '\000' || dataLen > (BUF_SIZE-1)) break;
    }

    letti += dataLen;

    printf("[+] RECEIVED: %d bytes.\n", letti);

        
    memset(buf, 0, BUF_SIZE);
    
    dataLen = 0;
    
/*
    printf("\n\nRICHIESTA 2 DI FILE: %s \n\n======== \n", req);
    
    while((bytes_read = (recv(fd_skt, buf, sizeof(buf), 0))) > 0){

        dataLen += bytes_read;

        if( buf[dataLen] == '\000' || dataLen > (BUF_SIZE-1)) break;
    }

    printf("\nRECEIVED: %s\n" , buf);
*/

    close(fd_skt);
    
    exit(EXIT_SUCCESS);

}
    


int openConnection(const char * sockname, int msec, const struct timespec abst){

    return 0;
}

int closeConnection(const char * sockname){
    
    return 0;
}
