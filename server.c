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
#include "./lib/icl_hash.h"

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

int keepRunning = 1;
int handledSuccessfully = 0;

int s_socket = -1;
int server_socket;  //-- Fd su cui si connettono i client.


/* Queue */
node_t * read_queue = NULL;

node_t * td_sockets = NULL;

icl_hash_t hash_map;

typedef struct _socket_used
{
    int socketN;
    int used; 

} socket_used;


socket_used socket_array[1024];

static pthread_mutex_t sk_mtx = PTHREAD_MUTEX_INITIALIZER;

static pthread_mutex_t rd_mtx = PTHREAD_MUTEX_INITIALIZER;

static pthread_mutex_t ctr_mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t td_mtx = PTHREAD_MUTEX_INITIALIZER;



static pthread_cond_t read_cond_var = PTHREAD_COND_INITIALIZER;
//Server-socket-set
static pthread_cond_t ssset_cond_var = PTHREAD_COND_INITIALIZER;


pthread_t thread_pool[THREAD_POOL_SIZE];

pthread_t monitor_thread;


static void Pthread_mutex_lock ( pthread_mutex_t *mtx)
{
    int err; 
    if( ( err = pthread_mutex_lock(mtx) != 0)){
        errno = err;
        perror("lock");
        pthread_exit((void *)(intptr_t) errno);
    }

    //Lock presa
}

static void Pthread_mutex_unlock ( pthread_mutex_t *mtx)
{
    int err; 
    if( ( err = pthread_mutex_unlock(mtx) != 0)){
        errno = err;
        perror("unlock");
        pthread_exit((void *)(intptr_t) errno);
    }

    //Unlocked! 
}


static void parse_configuration (struct configuration_t *);

void * connection_handler(void *);
void * thread_function(void *);
void * monitor_function(void *);

void intHandler() {
    keepRunning = 0;
    printf("Risolte: %d", handledSuccessfully);
    fflush(stdout);
    unlink(SOCKNAME);
    exit(0);
}

void start_server();
int shut_server();
void loop_server();
void destroy_threads();

int main(int argc, char const *argv[])
{
    
    if(argc > 0) {if(argv[0] == NULL) {} }  //--    ignorami :) 

    signal(SIGINT, intHandler);
    signal(SIGPIPE, SIG_IGN);

    start_server(); 

    parse_configuration(&configuration_parsed); //-- Parsing della configurazione.      
    
    loop_server(); //-- Main thread si blocca sulle connessioni in ingresso e distribuisce i task ai thread workers. 

    return shut_server();
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
        if(line[0] == '%' || strcmp(line, "\n") == 0 || strcmp(line, "\r\n") == 0) 
            continue;
        else { 

            value = strtol(strchr(strchr(line, '='), ' ') , &key, 10); 

            fflush(stdout);

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
                //Finito di leggere libero tutto.
                free(line);
                free(key);
                free(fptr); 
                fclose(fptr);
                break;
            }
        
        
        }

    }

    printf("\nMax dim: %d%s \nMax n. file:  %d\nThread worker: %d",  obj->max_dim, obj->bytes_order, obj->max_files, obj->worker_thread_number);
}


/**
 * @brief Gestione dei task. Ad ora, legge le richieste di file da parte del client e glieli restituisce sulla socket. 
 * 
 * @param p_client_socket sarebbe un int, ma il compilatore si lamenta, viene castato subito dopo. 
 * 
 * @return void* 
 */
void * connection_handler (void* p_client_socket) {
    
    int close_connection_flag = 0;

    int client_socket =  * ((int*)p_client_socket); 
    free(p_client_socket);  // -- corrisponde alla malloc in thread_function
    FILE *fp; 
    char buff[MAXLINE+1];
    char * path;; 
    int bytes_read;
    size_t dataLen; 

    while(!close_connection_flag){
        
        fp = NULL;
        path = NULL;
        bytes_read = 0;
        dataLen = 0;
        
        //Azzero il buffer.
        memset(buff, 0 , MAXLINE);  

        /**
         * Leggo il messaggio del client sulla socket, tengo traccia della quantità di byte rimasti da leggere, 
         * in caso il messaggio sia più lungo della dimensione del buffer.
         */
        
        while (!close_connection_flag && (bytes_read = read(client_socket, buff + dataLen, sizeof(buff) - dataLen - 1)) >= 0) {
            dataLen += bytes_read;
            //Se esco dai limiti del buffer o leggo un 'a capo' esco dal ciclo.  
            if(buff[dataLen - 1] == '\n' || dataLen > (MAXLINE-1)) break;

            if(bytes_read == 0 || bytes_read == -1 ) {close_connection_flag = 1; fprintf(stderr, "[X] Chiudo la socket! %d", client_socket);
}
        }

        if(!close_connection_flag){
            buff[dataLen-1] = 0;

            if((int) bytes_read == -1){ // -- Errore in lettura (Socket da chiudere).
                shutdown(client_socket, SHUT_RDWR);
                perror("Read: ");            
                break;

            } else { 
                /**
                 * Se mi mandano un path sbagliato fallisce, 
                 * e chiude la connessione
                 */
                if( (path = (realpath(buff, NULL))) == NULL){ 
                    shutdown(client_socket, SHUT_RDWR);
                    perror("RealPath: ");
                    break;
                }

                /**
                 * Come sopra. 
                 * Se fallisce chiudo tutto.
                 */


                fp = fopen(path, "r");
                if (fp == NULL) { 
                    perror("Errore nell'apertura del file: "); 
                    break;
                } 

                /**
                 * Tieni traccia del numero di bytes letti fino a questo momento, poiche' 
                 * se il file da inviare è più grande della dimensione del buffer lo invia in più mandate.
                 * Il client --> DEVE <-- mantenere la connessione aperta abbastanza a lungo per riceverlo tutto,
                 * altrimenti la write invoca un'eccezione per via della pipe rotta. 
                 */
                
                int n;

                while((bytes_read = fread(buff, sizeof(char), BUFSIZE, fp)) > 0)
                {
                    printf("\n[🛫]Invio %d bytes.\n", bytes_read);
                    fflush(stdout);
                    
                    
                    if((n =  write(client_socket, buff, bytes_read)) <= 0){
                        perror("[-] Write: ");
                        //Ignoro SIGPIPE e chiudo manualmente le connessioni lato server.
                        break;
                    }
                }
                
                fprintf(stderr, "\n[x] EOF\n");
                
                //Chiudo il file (TODO: va modificato per leggere in memoria, non da file system!)
                fclose(fp);

                //Libero la memoria usata per il path
                free(path);
                
                Pthread_mutex_lock(&ctr_mtx);
                handledSuccessfully++;
                Pthread_mutex_unlock(&ctr_mtx);
            }

        
        }
        else break;

    }

    
    fprintf(stderr, "[X] Chiudo la socket! %d", client_socket);
    close(client_socket);
    client_socket = -1;
    pthread_cond_signal(&read_cond_var);    //-- Segnalo ad un thread l'arrivo di un task


    return NULL;
}

/**
 * @brief Funzione che viene passata ai thread di default alla creazione.
 * 
 * @param arg per uso futuro
 * @return void* per essere conforme con le specifiche di pthread. 
 */
void * thread_function( void __attribute((unused)) * arg){

    //TODO: Aggiungere cond. di terminazione.
    while (1)
    {   
        int client; // -- numero fd socket
        int * p_client = malloc(sizeof(int)); // -- mantengo un puntatore per passarlo all'handler. 

        //Acquisisco la lock.
        Pthread_mutex_lock(&rd_mtx);
        
        //Se non c'è lavoro da fare mi metto in attesa.
        if((client = dequeue(&read_queue)) == -1){
            
            //E attendo un segnale per essere risvegliato, rilasciando la lock. 
            pthread_cond_wait(&read_cond_var, &rd_mtx);
            
            //Riprovo!
            client = dequeue(&read_queue);
        } 
        //Mollo la lock sulla coda.
        Pthread_mutex_unlock(&rd_mtx);

        //Se il thread è stato assegnato ad un task: 
        if(client != -1){
            
            //Passo alla funzione che gestisce i task il client socket.
            * p_client = client;

            //ed eseguo il lavoro.
            connection_handler(p_client);
        }
    }
}

/**
 * @brief   Procedura per far partire il server.
 *          Crea una threadpool e si occupa del binding della socket.
 * 
 * @return ** void 
 */
void start_server(){

    SA sockaddr;        //-- struct per la socket 

    //Creo la mia thread pool di workers. 
    for (int i = 0; i < THREAD_POOL_SIZE; i++)
    {
        pthread_create(&thread_pool[i], NULL, thread_function, NULL);
    } 

    
    if (( server_socket = socket(AF_UNIX, SOCK_STREAM, 0 )) < 0) //-- Creo la socket server.
        perror("Errore nella creazione della socket: ");
    
    sockaddr.sun_family = AF_UNIX;
    strncpy(sockaddr.sun_path, SOCKNAME, UNIX_PATH_MAX);

    if((bind(server_socket, (struct sockaddr *) &sockaddr, sizeof(sockaddr))) < 0)  //-- Binding della socket all'indirizzo.
        perror("Errore durante il binding: ");

    if((listen(server_socket, SERVER_QUEUE)) < 0)  //-- Si mette in ascolto - Il server ha un backlog di 100 connessioni. 
        perror("Errore durante l'ascolto della socket: ");

}

void loop_server(){
    
    int client_socket;
    
    while (1)
    {
        client_socket = accept(server_socket, NULL, 0);                                      //-- Si blocca sulla accept.
        fprintf(stderr, "\n[👋] Connessione in arrivo sul fd: %d! ", client_socket);    //-- e' arrivato una connessione (si sblocca)!
        
        if(client_socket == -1) perror("Accept(): ");                                   //-- errore di connessione.
        else{
            int * p_client = malloc(sizeof(int));   // -- creo un puntatore al fd per passarlo alla coda. 
            * p_client = client_socket;

            Pthread_mutex_lock(&rd_mtx);            //-- Ottengo la lock sulla coda
            enqueue(&read_queue, * p_client);       //-- Inserisco la socket del client nella lista
            print_queue(&read_queue);
            pthread_cond_signal(&read_cond_var);    //-- Segnalo ad un thread l'arrivo di un task
            Pthread_mutex_unlock(&rd_mtx);          //-- Sblocco la coda.
        }

    }
}

int shut_server(){
    printf("Chiudo!");
    unlink(SOCKNAME);
    return 0;
}

void destroy_threads(){
    for (int i = 0; i < THREAD_POOL_SIZE; i++)
    {
        
    }
    
}
