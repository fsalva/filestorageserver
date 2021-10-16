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

int keepRunning = 1;
int handledSuccessfully = 0;

/* Queue */
node_t * head = NULL;
node_t * bad_sockets = NULL;


static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t ctr_mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t bad_socket_mtx = PTHREAD_MUTEX_INITIALIZER;


static pthread_cond_t cond_var = PTHREAD_COND_INITIALIZER;
static pthread_cond_t remove_socket_cond_var = PTHREAD_COND_INITIALIZER;

pthread_t thread_pool[THREAD_POOL_SIZE];

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

void intHandler() {
    keepRunning = 0;
    printf("Risolte: %d", handledSuccessfully);
    fflush(stdout);
    unlink(SOCKNAME);
    exit(0);
}


int main(int argc, char const *argv[])
{
    signal(SIGINT, intHandler);


    


    //Creo la mia thread pool. 
    for (int i = 0; i < THREAD_POOL_SIZE; i++)
    {
        pthread_create(&thread_pool[i], NULL, thread_function, NULL);
    } 
    
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

    //-- socket attivi e socket pronti ad essere letti.
    fd_set curr_sockets, ready_sockets;
    int max_socket = server_socket;

    // -- timeout
    struct timeval timeout = {1, 0};

    int sel_result;
    int curr, temp; 

    //inizializzo
    FD_ZERO(&curr_sockets);
    FD_SET(server_socket, &curr_sockets);



    //Entro nel loop del server.
    //TODO: condizione d'uscita per spegnere tutto ordinatamente.  
    while(1){
        printf("\nAttendo connessioni... \n");

        ready_sockets = curr_sockets;

        if((sel_result = (select(max_socket + 1, &ready_sockets, NULL, NULL, &timeout))) < 0){
            perror("select error: ");
            //exit(EXIT_FAILURE);
        }
        if(sel_result == 0) {
            if((curr = dequeue(&bad_sockets)) == -1){
            
            //E attendo un segnale per essere risvegliato, rilasciando la lock. 
            pthread_cond_wait(&remove_socket_cond_var, &bad_socket_mtx);
            
            //Riprovo!
            curr = dequeue(&bad_sockets);
            } 

            //Mollo la lock sulla coda.
            Pthread_mutex_unlock(&bad_socket_mtx);

            //Se il thread è stato assegnato ad un task: 
            if(curr != -1){
                temp = curr; 
                close(temp); // -- chiudo la socket
                temp = -1; // -- e la setto a -1.

                FD_CLR(curr, &curr_sockets);
            }

            //Resetto il timer.
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;
        }
        

        else
        {
            for( int i = 0; i <= max_socket; i++){

                if(FD_ISSET(i, &ready_sockets)){
                    
                    if (i == server_socket){
                        
                        // -- c'è una nuova connessione
                        client_socket = accept(server_socket, NULL, 0);
                        FD_SET(client_socket, &curr_sockets);
                        if(client_socket == -1) perror("Accept(): ");
                        if(client_socket > max_socket) max_socket = client_socket;
                        fprintf(stderr, "\n[+] Una nuova connessione! \n    Socket FD number: %d ", client_socket);
                    } 
                    else {
                        fprintf(stderr, "\n[+] Client con socket ID: %d vuole scrivere! ", i);

                        // -- è un client pronto ad inviare dati
                        int * p_client = malloc(sizeof(int));
                        * p_client = i;

                        //Ottengo la lock sulla coda
                        Pthread_mutex_lock(&mtx);
                        //Inserisco la socket del client nella lista
                        enqueue(&head, * p_client);
                        //Segnalo ad un thread l'arrivo di un task
                        pthread_cond_signal(&cond_var);
                        //Sblocco la coda.
                        Pthread_mutex_unlock(&mtx);

                    }
                }
            }
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
    
    int client_socket =  * ((int*)p_client_socket); 
    free(p_client_socket);  // -- corrisponde alla malloc in thread_function

    FILE *fp = NULL;
    char buff[MAXLINE+1];
    char * path = NULL; 
    size_t bytes_read = -1;
    size_t dataLen = 0; 
    
    //Azzero il buffer.
    memset(buff, 0 , MAXLINE);  

    /**
     * Leggo il messaggio del client sulla socket, tengo traccia della quantità di byte rimasti da leggere, 
     * in caso il messaggio sia più lungo della dimensione del buffer.
     */
    
    while ((bytes_read = read(client_socket, buff + dataLen, sizeof(buff) - dataLen - 1)) > 0) {
        dataLen += bytes_read;
        //Se esco dai limiti del buffer o leggo un 'a capo' esco dal ciclo.  
        if(buff[dataLen - 1] == '\n' || dataLen > (MAXLINE-1)) break;
    }

    buff[dataLen-1] = 0;

    if((int) bytes_read == -1){ // -- Errore in lettura (Socket da chiudere).
        shutdown(client_socket, SHUT_RDWR);
        perror("Read: ");
        
        Pthread_mutex_lock(&bad_socket_mtx);
        enqueue(&bad_sockets, client_socket);
        Pthread_mutex_unlock(&bad_socket_mtx);

        pthread_cond_signal(&remove_socket_cond_var);


        return NULL;
    } else { 
        /**
         * Se mi mandano un path sbagliato fallisce, 
         * e chiude la connessione
         */
        if( (path = (realpath(buff, NULL))) == NULL){ 
            shutdown(client_socket, SHUT_RDWR);
            perror("RealPath: ");
            
            Pthread_mutex_lock(&bad_socket_mtx);
            enqueue(&bad_sockets, client_socket);
            Pthread_mutex_unlock(&bad_socket_mtx);

            pthread_cond_signal(&remove_socket_cond_var);


            return NULL;
        }

        /**
         * Come sopra. 
         * Se fallisce chiudo tutto.
         */


        fp = fopen(path, "r");
        if (fp == NULL) { 
            perror("Errore nell'apertura del file: "); 
            shutdown(client_socket, SHUT_RDWR);
            perror("FOPEN: ");

            Pthread_mutex_lock(&bad_socket_mtx);
            enqueue(&bad_sockets, client_socket);
            Pthread_mutex_unlock(&bad_socket_mtx);

            pthread_cond_signal(&remove_socket_cond_var);

            return NULL;
        } 

        /**
         * Tieni traccia del numero di bytes letti fino a questo momento, poiche' 
         * se il file da inviare è più grande della dimensione del buffer lo invia in più mandate.
         * Il client --> DEVE <-- mantenere la connessione aperta abbastanza a lungo per riceverlo tutto,
         * altrimenti la write invoca un'eccezione per via della pipe rotta. 
         */
        
        while((bytes_read = fread(buff, sizeof(char), BUFSIZE, fp)) > 0)
        {
            printf("Invio %zu bytes.\n", bytes_read);
            fflush(stdout);
            write(client_socket, buff, bytes_read);
        }


        //close(client_socket); 
        //client_socket = -1;
        
        //Chiudo il file (TODO: va modificato per leggere in memoria, non da file system!)
        fclose(fp);

        //Libero la memoria usata per il path
        free(path);
        

        Pthread_mutex_lock(&ctr_mtx);
        handledSuccessfully++;
        Pthread_mutex_unlock(&ctr_mtx);
    }

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
        Pthread_mutex_lock(&mtx);
        
        //Se non c'è lavoro da fare mi metto in attesa.
        if((client = dequeue(&head)) == -1){
            
            //E attendo un segnale per essere risvegliato, rilasciando la lock. 
            pthread_cond_wait(&cond_var, &mtx);
            
            //Riprovo!
            client = dequeue(&head);
        } 
        //Mollo la lock sulla coda.
        Pthread_mutex_unlock(&mtx);

        //Se il thread è stato assegnato ad un task: 
        if(client != -1){
            
            //Passo alla funzione che gestisce i task il client socket.
            * p_client = client;

            //ed eseguo il lavoro.
            connection_handler(p_client);
        }
    }
    
}
