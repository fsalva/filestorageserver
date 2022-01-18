#include "../server.h"
#include "../queue.h"
#include "../const.h"

#include<stdint.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include <errno.h>
// -- testing --
#include <dirent.h>

#include<pthread.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/time.h>

#define UNIX_PATH_MAX 108


int                 server_socket;  //-- Fd su cui si connettono i client.

node_t *            read_queue = NULL;
node_t *            td_sockets = NULL;

pthread_t           thread_pool;

pthread_mutex_t     sk_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t     rd_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t     ctr_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t     td_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t      read_cond_var = PTHREAD_COND_INITIALIZER;
pthread_cond_t      ssset_cond_var = PTHREAD_COND_INITIALIZER;

void *              thread_function( void __attribute((unused)) * arg);
void                clean_server(pthread_t *, int, int*);

/**
 * @brief Gestione dei task lato server.
 * 
 * @param p_client_socket 
 * @return void* 
 */
void * connection_handler(void * p_client_socket) {
    return NULL;
}

/**
 * @brief Ottiene la lock su una risorsa.
 * 
 * @param mtx 
 * @return ** void 
 */
void Pthread_mutex_lock ( pthread_mutex_t *mtx)
{
    int err; 
    if( ( err = pthread_mutex_lock(mtx) != 0)){
        errno = err;
        perror("lock");
        pthread_exit((void *)(intptr_t) errno);
    }

    //Lock presa
}

/**
 * @brief Rilascia la lock su una risorsa.
 * 
 * @param mtx 
 * @return ** void 
 */
void Pthread_mutex_unlock ( pthread_mutex_t *mtx)
{
    int err; 
    if( ( err = pthread_mutex_unlock(mtx) != 0)){
        errno = err;
        perror("unlock");
        pthread_exit((void *)(intptr_t) errno);
    }

    //Unlocked! 
}

/**
 * @brief Avvio del server 
 * -    creazione thread worker
 * -    messa in ascolto su una socket
 * 
 * @param workers_n numero di thread da creare (config)
 * @param mem_size dimensione massima della memoria (config)
 * @param files_n numero di files da salvare al massimo (config)
 * @param sock_name path alla socket (config)
 * @return int che rappresenta il risultato dell'operazione (successo / fail)
 */
int start_server(int workers_n, int mem_size, int files_n, char * sock_name){

    SA              sockaddr;
    int *           index;
    pthread_t *     ptr;

    

    index = calloc (workers_n, sizeof (int));
    for(int i = 0; i < workers_n; i++)
    {
        index[i] = i;
    }

    ptr = malloc(sizeof(pthread_t)*workers_n);
    for(int i = 0; i < workers_n; i++)
    {
        pthread_create(&ptr[i], NULL, thread_function, (void*)&index[i]);
    }

    if (( server_socket = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) //-- Creo la socket server.
    {
        perror("[error]\t[start_server]:\tErrore nella creazione della socket: ");
        return -1;
    }
    
    
    // Azzero la struttura.
    memset(&sockaddr, 0, sizeof(struct sockaddr_un));
    sockaddr.sun_family = AF_UNIX;
    strncpy(sockaddr.sun_path, sock_name, UNIX_PATH_MAX);


    // -- testing --
    int error;

    if((error = (bind(server_socket, (struct sockaddr *) &sockaddr, sizeof(sockaddr)))) < 0)  //-- Binding della socket all'indirizzo.
    {
        fprintf(stderr, "\n\nERRCODE: %d\n\nAvevo socket fd : %d", error, server_socket);
        perror("[error]\t[start_server]:\tErrore durante il binding ");
        return -1;
    }

    if((listen(server_socket, SERVER_QUEUE)) < 0)  //-- Si mette in ascolto - Il server ha un backlog di 100 connessioni. 
    {
        perror("[error]\t[start_server]:\tErrore durante l'ascolto della socket:: ");
        return -1;
    }


    
    
    return 0;

}

void clean_server(pthread_t * ptr, int workers_n, int * index){

    fprintf(stderr, "[info]\t[clean_server]: Libero spazio");

    for (int i = 0; i < workers_n; i++)
    {
        free((pthread_t *) ptr[i]);
    }
    
    free(ptr);
    free(index);

}

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

void loop_server(){
    
    int client_socket;
    
    while (1)
    {
        client_socket = accept(server_socket, NULL, 0);                                 //-- Si blocca sulla accept.
        fprintf(stderr, "\n[👋] Connessione in arrivo sul fd: %d! ", client_socket);    //-- e' arrivato una connessione (si sblocca)!
        
        if (client_socket == -1) perror("Accept(): ");                                   //-- errore di connessione.
        else {
            int * p_client = malloc(sizeof(int));   // -- creo un puntatore al fd per passarlo alla coda. 
            * p_client = client_socket;

            Pthread_mutex_lock(&rd_mtx);            //-- Ottengo la lock sulla coda
            enqueue(&read_queue, * p_client);       //-- Inserisco la socket del client nella lista
            pthread_cond_signal(&read_cond_var);    //-- Segnalo ad un thread l'arrivo di un task
            Pthread_mutex_unlock(&rd_mtx);          //-- Sblocco la coda.
        }

    }
}
