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

#include "./lib/fsqueue.h"
#include "./lib/constvalues.h"

//Per abbreviare il codice.
#define SA struct sockaddr_un
#define CP configuration_parsed



/* Commentare e decommentare per eseguire parti di codice. */
#define thread
#define config

//TODO: Sposta su libreria.
    struct configuration_t
    {
        int worker_thread_number;
        int max_dim;
        int max_files;
        char * bytes_order;

    } configuration_parsed;


int ACTIVE_SOCKET = 0;
int keepRunning = 1;

#ifdef thread

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_t pool[THREAD_POOL_SIZE];


static void Pthread_mutex_lock ( pthread_mutex_t *mtx)
{
    int err; 
    if( ( err = pthread_mutex_lock(mtx) != 0)){
        errno = err;
        perror("lock");
        pthread_exit((void *)(intptr_t) errno);
    }

    else printf("\n [ Locked ]");
}

static void Pthread_mutex_unlock ( pthread_mutex_t *mtx)
{
    int err; 
    if( ( err = pthread_mutex_unlock(mtx) != 0)){
        errno = err;
        perror("unlock");
        pthread_exit((void *)(intptr_t) errno);
    }

    else fprintf(stdout, "\n [ Unlocked ]");
    fflush(stdout);
}

#endif


static void parse_configuration (struct configuration_t *);

void * connection_handler(int *);


void intHandler() {
    keepRunning = 0;
    close(ACTIVE_SOCKET);
    unlink(SOCKNAME);
    exit(0);
}

int main(int argc, char const *argv[])
{
    signal(SIGINT, intHandler);

    #ifdef thread
    
    /* Thread */
    pthread_t t; 

/* 
    for (int i = 0; i < THREAD_POOL_SIZE; i++)
    {
        pthread_create(&t, NULL, connection_handler, NULL);
    } */
    

    //int err, status;

    #endif

    /* Variabili per la server socket */
    int server_socket, client_socket;
    SA sockaddr;

    //TODO: Rimuovere il warning.
    if(argc > 0) {if(argv[0] == NULL) {} }

    #ifdef config
    
    /**
     * Esegue il parsing della configurazione del server, e lo 
     * restituisce nella struttura dati sopra. 
     */
    parse_configuration(&configuration_parsed);    
    
    #endif
     
    if (( server_socket = socket(AF_UNIX, SOCK_STREAM, 0 )) < 0) //Creo una socket.
        perror("Errore nella creazione della socket: ");
    
    sockaddr.sun_family = AF_UNIX;
    strncpy(sockaddr.sun_path, SOCKNAME, UNIX_PATH_MAX);

    if((bind(server_socket, (struct sockaddr *) &sockaddr, sizeof(sockaddr))) < 0)    //Binding della socket all'indirizzo.
        perror("Errore durante il binding: ");

    if((listen(server_socket, SERVER_QUEUE)) < 0)  //Si mette in ascolto.
        perror("Errore durante l'ascolto della socket: ");



    //Entro nel loop del server.
    //TODO: condizione d'uscita per spegnere tutto ordinatamente.  
    while(1){

        printf("Attendo connessioni... \n");

        //Si blocca in attesa di connessioni, restituisce un file descriptor. 
        client_socket = accept(server_socket, NULL, 0);

        if(client_socket == -1)
            perror("Accept(): ");
        else 
        {
            int * p_client = malloc(sizeof(int));
            * p_client = client_socket;

            pthread_create(&t, NULL, connection_handler, p_client);
            
        }

    }

    printf("Chiudo!");
    unlink(SOCKNAME);
    return 0;
}

static void parse_configuration (struct configuration_t * obj){
    
/* File di configurazione */
    FILE *fptr;
    char * line = NULL;
    char * key;
    long value;
    size_t len = 0;
    ssize_t readQnt;

    int counter = 0;

if ((fptr = fopen(CONFIG_PATH ,"r")) == NULL){
        //Se non riesco a leggere la configurazione da file, esco.
        perror("[Lettura configurazione]");
        exit(-1);
    }


    while ((readQnt = getline(&line, &len, fptr)) != -1) {
        if(line[0] == '%') continue;
        else { 

            value = strtol(strchr(strchr(line, '='), ' ') , &key, 10); 
            switch (counter)
            {
            case 0:
                obj->worker_thread_number = value; counter++;
                break;

            case 1:
                obj->max_dim = value; 
                /* *  
                * Alloco lo spazio che mi serve per dichiarare una stringa che contenga l'ordine
                * di grandezza scritto in configurazione. +1 per il carattere di terminazione 
                * della stringa '\0. 
                * */
                obj->bytes_order = strdup(key); //E ci copio dentro la stringa
                counter++;
                break;

            case 2:
                obj->max_files = value; counter++;
                break;

            default:
                free(line);
                free(key);
                free(fptr); 
                fclose(fptr);
                break;
            }
        }
    }

    


}

void * connection_handler (int* p_client_socket) {
    
    int client_socket =  * p_client_socket; 
    free(p_client_socket);

    ACTIVE_SOCKET = client_socket;

    char buff[MAXLINE+1];
    char * path = NULL; 
    size_t bytes_read;
    size_t dataLen = 0; 

    //Azzero il buffer.
    memset(buff, 0 , MAXLINE);  

    //Leggo il messaggio del client.
    while ((bytes_read = read(client_socket, buff + dataLen, sizeof(buff) - dataLen - 1)) > 0)
    {
       dataLen += bytes_read;
       //Se esco dai limiti del buffer o leggo un 'a capo' esco dal ciclo.  
       if(buff[dataLen - 1] == '\n' || dataLen > (MAXLINE-1)) break;

    }

    buff[dataLen-1] = 0;

    if((int) bytes_read == -1) perror("Errore in lettura: ");   

    printf("Client requests: %s\n", buff);
    fflush(stdout);

    /**
     * Se mi mandano un path sbagliato fallisce, 
     * e chiude la connessione
     */
    if( (path = (realpath(buff, NULL))) == NULL){ 
        perror("RealPath: "); 
        close(client_socket); 
        return NULL;
    }

    /**
     * Come sopra. 
     * Se fallisce chiudo tutto.
     */

    FILE *fp = fopen(path, "r");
    if (fp == NULL) { 
        perror("Errore nell'apertura del file: "); 
        close(client_socket); 
        return NULL;
    } 

    while((bytes_read = fread(buff, sizeof(char), BUFSIZE, fp)) > 0)
    {
        printf("Invio %zu bytes.\n", bytes_read);
        fflush(stdout);
        write(client_socket, buff, bytes_read);
        perror("write: ");

    }



    free(path);
    close(client_socket);
    perror("Close: ");
    fclose(fp);

    return NULL;
}
