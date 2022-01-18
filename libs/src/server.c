#include "../server.h"
#include "../queue.h"
#include "../const.h"

#include<stdint.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include <errno.h>

#include<pthread.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/time.h>

pthread_mutex_t sk_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t rd_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ctr_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t td_mtx = PTHREAD_MUTEX_INITIALIZER;pthread_cond_t read_cond_var = PTHREAD_COND_INITIALIZER;
pthread_cond_t ssset_cond_var = PTHREAD_COND_INITIALIZER;

pthread_t thread_pool;

node_t * read_queue = NULL;
node_t * td_sockets = NULL;

int server_socket;  //-- Fd su cui si connettono i client.

void * thread_function( void __attribute((unused)) * arg);
void clean_server(pthread_t *, int, int*);

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

    SA sockaddr;
    int * index;
    pthread_t * ptr;

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

    if (( server_socket = socket(AF_UNIX, SOCK_STREAM, 0 )) < 0) //-- Creo la socket server.
    {
        perror("[error]\t[start_server]:\tErrore nella creazione della socket: ");
        clean_server(ptr, workers_n, index);
        return -1;
    }

    sockaddr.sun_family = AF_UNIX;
    strncpy(sockaddr.sun_path, sock_name, UNIX_PATH_MAX);

    fprintf(stderr, "Sockaddr.sun_family = %d \n sock_name =  %s", sockaddr.sun_family, sockaddr.sun_path);

    if((bind(server_socket, (struct sockaddr *) &sockaddr, sizeof(sockaddr))) < 0)  //-- Binding della socket all'indirizzo.
    {
        perror("[error]\t[start_server]:\tErrore durante il binding: ");
        clean_server(ptr, workers_n, index);
        return -1;
    }

    if((listen(server_socket, SERVER_QUEUE)) < 0)  //-- Si mette in ascolto - Il server ha un backlog di 100 connessioni. 
    {
        perror("[error]\t[start_server]:\tErrore durante l'ascolto della socket:: ");
        clean_server(ptr, workers_n, index);
        return -1;
    }


    
    
    return 0;

}

void clean_server(pthread_t * ptr, int workers_n, int * index){

    fprintf(stderr, "[info]\t[clean_server]: Ripulendo.");

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
