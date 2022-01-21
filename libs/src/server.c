#include "../server.h"
#include "../queue.h"
#include "../const.h"
#include "../stringutils.h"
#include "../request.h"
#include "../prettyprint.h"
#include "../supported_operations.h"
#include "../icl_hash.h"

#include<stdint.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<errno.h>

#include<pthread.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/time.h>

#define UNIX_PATH_MAX 108


                
node_t *            read_queue = NULL;
node_t *            td_sockets = NULL;

pthread_t           thread_pool;

pthread_mutex_t     sk_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t     rd_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t     ctr_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t     td_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t      read_cond_var = PTHREAD_COND_INITIALIZER;
pthread_cond_t      ssset_cond_var = PTHREAD_COND_INITIALIZER;

icl_hash_t *        hashtable;


void *              thread_function( void __attribute((unused)) * arg);
void                clean_server(pthread_t *, int, int*);


/**
 * @brief Gestione dei task lato server.
 * 
 * @param p_client_socket 
 * @return void* 
 */
void * connection_handler(void * p_client_socket) {
    
    int         close_connection_flag = 0;
    int         client_socket;
    int         bytes_read;

    char        buff[MAXLINE+1];
    char *      path;; 
    
    FILE *      fp; 
    size_t      dataLen;
    
    request * req = NULL; 


    client_socket = * ((int*)p_client_socket); 
    free(p_client_socket);  // -- corrisponde alla malloc in thread_function
    

    //Azzero il buffer.
    memset(buff, 0 , MAXLINE);

    while(!close_connection_flag){

        fp = NULL;
        path = NULL;
        bytes_read = 0;
        dataLen = 0;

        /**
         * Leggo il messaggio del client sulla socket, tengo traccia della quantità di byte rimasti da leggere, 
         * in caso il messaggio di richiesta sia più lungo della dimensione del buffer.
         */
        
        while (!close_connection_flag && (bytes_read = read(client_socket, buff + dataLen, sizeof(buff) - dataLen - 1)) >= 0) {
            dataLen += bytes_read;
            //Se esco dai limiti del buffer o leggo un 'a capo' esco dal ciclo.  
            if(buff[dataLen - 1] == '\n' || dataLen > (MAXLINE-1)) break;

            if(bytes_read == 0 || bytes_read == -1 ) {close_connection_flag = 1;}
        }

        if(!close_connection_flag){

            buff[dataLen-1] = 0;

            if((int) bytes_read == -1){ // -- Errore in lettura (Socket da chiudere).
                shutdown(client_socket, SHUT_RDWR);
                perror("Read: ");            
                break;

            } else {    // Lettura avvenuta correttamente. 

                req = parse_request(buff);

                switch (req->r_op_code)
                {
                case OP_READ_FILE:

                    /**
                     * Se mi mandano un path sbagliato fallisce, 
                     * e chiude la connessione
                     
                    path = malloc(sizeof(char) * strlen(req->r_path));
                    
                    if( (path = (realpath(req->r_path, NULL))) == NULL){ 
                        shutdown(client_socket, SHUT_RDWR);
                        perror("RealPath:");
                        break;
                    }
                    */

                    //fprintf(stderr, "\nPath: {%s}", path);

                    /**
                     * Come sopra. 
                     * Se fallisce chiudo tutto.
                     */


                    fp = fopen(req->r_path, "r");
                    if (fp == NULL) { 
                        perror("Errore nell'apertura del file: "); 
                        break;
                    } 
                    //fprintf(stderr, "\n\tOperation code: %ld \n\tRequester PID: %ld\n\tRequest Path: [%s]\n", req->r_op_code, req->r_pid, req->r_path);

                    /**
                     * Tieni traccia del numero di bytes letti fino a questo momento, poiche' 
                     * se il file da inviare è più grande della dimensione del buffer lo invia in più mandate.
                     * Il client --> DEVE <-- mantenere la connessione aperta abbastanza a lungo per riceverlo tutto,
                     * altrimenti la write invoca un'eccezione per via della pipe rotta. 
                     */
                    
                    int n;
                    
                    printf("\n[🛫]Invio il file al client: %d.\n", client_socket);

                    while((bytes_read = fread(buff, sizeof(char), BUFSIZE, fp)) > 0)
                    {
                        //fflush(stdout);
                        
                        
                        if((n =  write(client_socket, buff, bytes_read)) <= 0){
                            perror("[-] Write: ");
                            //Ignoro SIGPIPE e chiudo manualmente le connessioni lato server.
                            break;
                        }
                    }

                    write(client_socket, "\000", sizeof("\000"));
                    
                    
                    //Chiudo il file (TODO: va modificato per leggere in memoria, non da file system!)
                    fclose(fp);

                    //Libero la memoria usata per il path
                    free(path);
                    
                    Pthread_mutex_lock(&ctr_mtx);
                    handledSuccessfully++;
                    Pthread_mutex_unlock(&ctr_mtx);
                    break;

                case OP_WRITE_FILES:
                    write_file(req->r_path, req->r_pid, client_socket, hashtable);
                    break;

                default:
                    break;
                }
            }
        }
        else break;
    }

    
    //fprintf(stderr, "[X] Chiudo la socket! %d", client_socket);
    close(client_socket);
    client_socket = -1;
    pthread_cond_signal(&read_cond_var);    //-- Segnalo ad un thread l'arrivo di un task


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

    hashtable = icl_hash_create(16, hash_pjw, string_compare);

    handledSuccessfully = 0;

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
    trim(sock_name);  //    Necessario per evitare il bug dei caratteri aggiunti ('$'\n).
    strncpy(sockaddr.sun_path, sock_name, strlen(sock_name));

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
        //fprintf(stderr, "\n[👋] Connessione in arrivo sul fd: %d! ", client_socket);    //-- e' arrivato una connessione (si sblocca)!
        
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

