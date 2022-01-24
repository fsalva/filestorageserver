/**
 * @file clientapi.c
 * @author Francesco Salvatori
 * @brief API per permettere al client di connettersi al server con le operazioni di base.
 * @version 0.1
 * @date 2022-01-24
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#define _POSIX_C_SOURCE 199309L
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <string.h>

#include "../clientapi.h"
#include "../prettyprint.h"


bool running = true;
bool connected = false;
int debug_flaggg = 1;

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

/**
 * @brief Il thread fa una lock ed aspetta il segnale dell'avvenuta connessione. Aspetta per il tempo specificato da abs_t. 
 * 
 * @param abs_t 
 * @return void* 
 */
static void * thread_function(void *abs_t) {
    struct timespec *abs = (struct timespec *) abs_t;

    pthread_mutex_lock(&lock);
    pthread_cond_timedwait(&cond, &lock, abs);

    running = false;
    pthread_mutex_unlock(&lock);
    return NULL;
}   

int openConnection(const char* sockname, int msec, const struct timespec abstime)
{

    int                 fd_skt;
    struct sockaddr_un  sa;

    pthread_t tid;

    memset(&sa, 0, sizeof(struct sockaddr_un));
    strncpy(sa.sun_path, sockname, 108);
    sa.sun_family=AF_UNIX;

    fd_skt = socket(AF_UNIX, SOCK_STREAM, 0);
  
    struct timeval now;
    struct timespec time_to_stop;

    gettimeofday(&now, NULL);   // Prende il tempo 'attuale'
                                            // TODO: 
    time_to_stop.tv_sec = now.tv_sec + 3;   // Ci aggiunge il tempo che dovrò specificare su abstime (default = 5s?) 
    time_to_stop.tv_nsec = now.tv_usec * 1000;   

    pthread_create(&tid, NULL, &thread_function, (void *) &time_to_stop);
    
    // Se il server non risponde immediatamente
    while(running)
    {   
        // Se abbiamo ancora tempo (abstime)
        if((connect(fd_skt, (struct sockaddr *) &sa, sizeof(sa)) < 0)){
            
            // Aspetta per "msec" millisecondi, poi riprova.
            usleep(msec * 1000);

        }
        else {
            // è connesso.
            pthread_cond_signal(&cond);
            pthread_join(tid, NULL);
            return fd_skt;
        }
         
    }

    pthread_join(tid,NULL);
    return -1;
}