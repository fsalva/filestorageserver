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

#include "./lib/constvalues.h"

//Per abbreviare il codice.
#define SA struct sockaddr_un
#define CP configuration_parsed



/* Commentare e decommentare per eseguire parti di codice. */
//#define thread
#define config

//TODO: Sposta su libreria.
    struct configuration_t
    {
        int worker_thread_number;
        int max_dim;
        int max_files;
        char * bytes_order;

    } configuration_parsed;

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static int x;
int ACTIVE_SOCKET = 0;

static void Pthread_mutex_lock ( pthread_mutex_t *mtx)
{
    int err; 
    if( ( err = pthread_mutex_lock(mtx) != 0)){
        errno = err;
        perror("lock");
        pthread_exit(errno);
    }

    else printf("\n [ Locked ]");
}

static void Pthread_mutex_unlock ( pthread_mutex_t *mtx)
{
    int err; 
    if( ( err = pthread_mutex_unlock(mtx) != 0)){
        errno = err;
        perror("unlock");
        pthread_exit(errno);
    }

    else printf("\n [ Unlocked ]");
}

static void* myFun (void* arg){
    while (x < (int) arg) {
        Pthread_mutex_lock(&mtx);
        printf("\nSecondo thread: x = %d\n", ++x);
        fflush(stdout);
        Pthread_mutex_unlock(&mtx);
        sleep(1);
    }

    pthread_exit((void *) 17); /* === a return (void*) 17*/
}

static struct configuration_t parse_configuration (struct configuration_t *);

int connection_handler(int);

void intHandler(int dummy) {
    close(ACTIVE_SOCKET);
    unlink(SOCKNAME);
    exit(0);
}

int main(int argc, char const *argv[])
{
    signal(SIGINT, intHandler);

    /* Thread */
    pthread_t tid; 
    int err, status;
    
    /* Variabili per la server socket */
    int server_socket, client_socket, n;
    SA sockaddr;
    socklen_t addr_len;
    char client_address[MAXLINE +1];

    int sendbytes;
    

 
    #ifdef config
        parse_configuration(&configuration_parsed);

        printf("%d%s", CP.max_dim, CP.bytes_order);

        
    #endif



    #ifdef thread 

        assert(sizeof(int)<= sizeof(void*)); 

        if (( err=pthread_create(&tid, NULL, &myFun, (void *) 10 ) != 0 ))
        {   
            /* Gestione dell'errore */

        } 
        else
        {   
            /* Il secondo thread viene creato. */
            while(x < 10){
                printf("\nPrimo thread x = %d\n", ++x);
                fflush(stdout);
                sleep(1);
                
            }
            pthread_join(tid,(void*) &status);
            printf("\n ** Thread 2 termina >> status code: %d\n", status);
            fflush(stdout);
        }
        
    #endif

     
    if (( server_socket = socket(AF_UNIX, SOCK_STREAM, 0 )) < 0) //Creo una socket.
        perror("Errore nella creazione della socket: ");
    
    sockaddr.sun_family = AF_UNIX;
    strncpy(sockaddr.sun_path, SOCKNAME, UNIX_PATH_MAX);

    if((bind(server_socket, (SA *) &sockaddr, sizeof(sockaddr))) < 0)    //Binding della socket all'indirizzo.
        perror("Errore durante il binding: ");

    if((listen(server_socket, SERVER_QUEUE)) < 0)  //Si mette in ascolto.
        perror("Errore durante l'ascolto della socket: ");


    //Entro nel loop del server.
    //TODO: condizione d'uscita per spegnere tutto ordinatamente.  
    while(1){

        printf("Attendo connessioni... \n");
        fflush(stdout);

        //Si blocca in attesa di connessioni, restituisce un file descriptor. 
        client_socket = accept(server_socket, NULL, 0);
        
        if(client_socket < 0)
            perror("Errore durante l'accettazione della connessione!");


        connection_handler(client_socket);




    }


    return 0;
}

static struct configuration_t parse_configuration (struct configuration_t * obj){
    
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
                char * make_space = (char *) malloc(sizeof(char) * (strlen(key) + 1));
                strcpy(make_space, key); //E ci copio dentro la stringa
                obj->bytes_order = make_space; counter++;
                break;

            case 2:
                obj->max_files = value; counter++;
                break;

            default:
                break;
            }
        }
    }
    fclose(fptr); 

}

int connection_handler (int client_socket) {
    
    
    ACTIVE_SOCKET = client_socket;

    int n;
    char * buff[MAXLINE+1];
    char * recvline[MAXLINE+1];
    char * path = NULL; 
    char * requested[MAXLINE +1];
    size_t bytes_read;
    size_t dataLen = 0; 
    ssize_t b;

    //Azzero il buffer.
    memset(recvline, 0 , MAXLINE);  

    //Leggo il messaggio del client.
    while ((bytes_read = read(client_socket, recvline + dataLen, sizeof(recvline) - dataLen - 1)) > 0)
    {
       dataLen += bytes_read;
       //Se esco dai limiti del buffer o leggo un 'a capo' esco dal ciclo.  
       if(dataLen > MAXLINE -1 || recvline[dataLen -1] == '\n') break;
    }

    if(bytes_read == -1) perror("Errore in lettura: ");   

    printf("Client requests: %s", recvline);

    /**
     * Se mi mandano un path sbagliato fallisce, 
     * e chiude la connessione
     */
    if( (path = (realpath("./testfiles/lipsum.txt", NULL))) == NULL){ 
        perror("RealPath: "); 
        close(client_socket); 
        return -1;
    }

    printf("Path: %s", path);
    fflush(stdout);

    /**
     * Come sopra. 
     * Se fallisce chiudo tutto.
     */
    FILE *fp = fopen(path, "r");
    if (fp == NULL) { 
        perror("Errore nell'apertura del file: "); 
        close(client_socket); 
        return -1;
    }

    while(( bytes_read = fread(buff, 1, BUFSIZE, fp)) > 0 ){
        printf("\nsending %zu bytes\n", bytes_read);
        fflush(stdout);
        if(write(client_socket, buff, bytes_read) == -1){
            perror("Write: ");
            close(client_socket);
            return -1;
        }

    }

    free(path);
    close(client_socket);
    fclose(fp);

    return 0;
}