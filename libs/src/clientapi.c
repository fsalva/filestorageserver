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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
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
#include "../const.h"
#include "../constvalues.h"
#include "../prettyprint.h"
#include "../stringutils.h"
#include "../supported_operations.h"

char *  format_request(char*, int, int);    // Formatta la richiesta in modo leggibile per il server.
int     send_request(int, int, char **);   // Invia la richiesta formattata al server e riceve un codice di risposta.

int     fd_skt;
int     this_pid;
int     debug_flaggg = 1;   // Todo: Rimuovi e metti negli args.
bool    running = true;
bool    connected = false;


static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

/**
 * @brief Il thread fa una lock ed aspetta il segnale dell'avvenuta connessione. Aspetta per il tempo specificato da abs_t. 
 * 
 * @param abs_t 
 * @return void* 
 */
static void * 
thread_function(void *abs_t) {
    struct timespec *abs = (struct timespec *) abs_t;

    pthread_mutex_lock(&lock);
    pthread_cond_timedwait(&cond, &lock, abs);
    print_debug("SBLOCCATO!", 1);
    running = false;
    pthread_mutex_unlock(&lock);
    return NULL;
}  

/**
 * @brief Apre la connessione sulla socket specificata, e invoca un thread per controllare il timeout.
 *         
 * @param sockname 
 * @param msec 
 * @param abstime 
 * @return identificatore del fd, o -1 se ci sono errori.
 */
int 
openConnection(const char* sockname, int msec, const struct timespec abstime)
{
    struct sockaddr_un  sa;

    pthread_t tid;

    memset(&sa, 0, sizeof(struct sockaddr_un));


    strcpy(sa.sun_path, "./socket/l.sock");

    sa.sun_family=AF_UNIX;

    fd_skt = socket(AF_UNIX, SOCK_STREAM, 0);
  
    struct timeval now;
    struct timespec time_to_stop;

    gettimeofday(&now, NULL);   // Prende il tempo 'attuale'
                                            // TODO: 
    time_to_stop.tv_sec = now.tv_sec + 1;   // Ci aggiunge il tempo che dovrò specificare su abstime (default = 5s?) 
    time_to_stop.tv_nsec = now.tv_usec + 500 * 1000;   

    pthread_create(&tid, NULL, &thread_function, (void *) &time_to_stop);
    
    // Se il server non risponde immediatamente
    while(running)
    {   
        // Se abbiamo ancora tempo (abstime)
        if((connect(fd_skt, (struct sockaddr *) &sa, sizeof(sa)) < 0)){
            
            // Aspetta per "msec" millisecondi, poi riprova.
            usleep(msec * 1000);
            printf("ops");
        }
        
            // è connesso.
            pthread_cond_signal(&cond);
            pthread_join(tid, NULL);
            return fd_skt;
        
    }

    pthread_join(tid,NULL);
    return -1;
}

int 
closeConnection(const char * sockname){
    
    /* Se il nome della socket matcha quello attuale:
    /   - Invia una richiesta al server per chiudere
    /   - Se il server accetta (Attendi risposta sulla socket)
    /   - Chiude lui e chiude il server. 
    */
    int     response;

    this_pid = getpid();     

    response = send_request(this_pid, CLOSE_CONNECTION, str_split((char *) sockname, '\0'));

    if(response == 0) {  // Match della socket, il server fa l'ACK della chiusura ordinata
        close(fd_skt);
        return 0;

    }
    
    return -1;      // TODO: Setta errno
}

int 
writeFile(const char * pathname, const char * dirname){
    
    int     n, response;
    char *  request_body;
    char *  buff = (char * ) malloc(sizeof(char) * (BUFSIZE + 1));
    char    f_size[256] = "";
    size_t  bytes_read;
    size_t  f_dim;
    FILE *  fp = NULL;


    if((fp = fopen(pathname, "r")) == NULL){
        perror("fopen");
        return -1;
    }

    f_dim = get_file_size(fp); 
    snprintf(f_size, sizeof f_size, "%zu", f_dim);

    this_pid = getpid();
    request_body = (char *) malloc( ( strlen(pathname) + strlen(dirname) + strlen(f_size) + 1) * sizeof(char) );



    formatStr(request_body, 3, f_size, pathname, dirname);

    response = send_request(this_pid, OP_WRITE_FILES, str_split(request_body, '\0'));

    fprintf(stderr, "\nServer risponde: %d", response);

    memset(buff, 0 , MAXLINE);

    // Inviare il file di ritorno.
    if(response >= 0){
        
        while((bytes_read = fread(buff, sizeof(char), BUFSIZE, fp)) > 0) {
            
            if((n =  write(fd_skt, buff, bytes_read)) <= 0){
                perror("[-] Write: ");
                break;
            }
        }

        write(fd_skt, "\000", sizeof("\000"));
    }
    
    free(request_body);
    free(buff);
    fclose(fp);

    return response;
}

int 
openFile(const char * pathname, int flags){
    
    int     response;
    char *  request_body; 
    char    flag_str[sizeof(int) + 2];

    // Inserisco il valore dei flag nella stringa. 
    sprintf(flag_str, "%d", flags);

    this_pid = getpid();    
    
    // Alloco spazio per tutta la stringa (e la formatto).
    request_body = (char *) malloc( ( strlen(pathname) + strlen(flag_str) + 1) * sizeof(char) );

    formatStr(request_body, 2, pathname, flag_str);
    
    // Invio la richiesta al server.
    response = send_request(this_pid, OP_OPEN_FILE, str_split(request_body, '\0'));

    free(request_body);
    
    return response;
}
char * 
format_request(char* request_body, int opt, int pid){

    char * fn = (char *) malloc(sizeof(char) * 255);
    char * op_str = (char *) malloc(sizeof(char) * 4);
    sprintf(op_str, "%d", opt);
    
    char pid_str[sizeof(int) + 2];
    sprintf(pid_str, "%d", pid);

    /**
     * [OPCODE]#[pid]#[request body] 
     * 
     * Segue ^ questo modello per formattare le richieste. 
    */

    sprintf(fn,"%d#%d#%s\n", opt, pid, request_body);

    return fn;
}


int 
send_request(int pid, int opt, char ** arguments){

    int         i;
    char *      request = NULL;
    char        buf[BUFSIZE];

    ssize_t bytes_read;
    size_t dataLen;

    if(arguments){

        // Per ogni argomento della richiesta: 
        for (i = 0; *(arguments + i); i++) {   
            request = format_request(*(arguments + i), opt, pid); // Formattalo in una singola richiesta: 
            
            //print_debug(request, 1);

            //debug.
            bytes_read = write(fd_skt, request, strlen(request));    // Ed invialo sulla socket.

            fprintf(stderr, "Inviati %zu bytes", bytes_read);

            memset(buf, 0, BUFSIZE);

            dataLen = 0;    // quantità di bytes ricevuti / buffer
            
            // TODO: 
            // Per ora fa una conta dei bytes ricevuti in risposta.
            while((bytes_read = recv(fd_skt, buf, sizeof(buf), 0)) >= 0){
                
                dataLen += bytes_read;  // Tiene traccia del numero di bytes letti, per controllare se va in overflow.
                
                //TODO: togliere messaggi di debug
                fprintf(stderr, buf);

                if(dataLen > (BUFSIZE-1)){
                    memset(buf, 0, BUFSIZE);
                    dataLen = 0;
                    
                } else if( buf[dataLen] == '\000') break; 
            }

            free(*(arguments + i));
        }

        printf("\n");

        return 0;
    }

    return -1;

}

