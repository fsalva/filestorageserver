                    
#include "../server.h"
#include "../queue.h"
#include "../const.h"
#include "../stringutils.h"
#include "../request.h"
#include "../prettyprint.h"
#include "../supported_operations.h"
#include "../icl_hash.h"
#include "../constvalues.h"
#include "../myfile.h"

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

int                 server_socket;
int                 handledSuccessfully;
int                 memory_size;
int                 files_amount;

node_t *            read_queue = NULL;
node_t *            td_sockets = NULL;

pthread_t           thread_pool;

pthread_mutex_t     sk_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t     rd_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t     ctr_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t     td_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t      read_cond_var = PTHREAD_COND_INITIALIZER;
pthread_cond_t      ssset_cond_var = PTHREAD_COND_INITIALIZER;

icl_hash_t *        hashtable = NULL;


int                 send_response(int, int, char *);
void                clean_server(pthread_t *, int, int*);
void *              thread_function( void __attribute((unused)) * arg);

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
    char        buf [MAXLINE+1];

    char *      path = NULL; 
    char *      dir  = NULL;

    char *      saveptr = NULL;

    void *      found = NULL;
    void *      content = NULL;
    
    size_t      f_dim = 0;
    size_t      dataLen;
    
    request *   req = NULL; 

    client_socket = * ((int*)p_client_socket); 
    free(p_client_socket);  // -- corrisponde alla malloc in thread_function

    //Azzero il buffer.
    memset(buff, 0 , sizeof(buff));

    while(!close_connection_flag){
        
        // LETTURA DEL BUFFER 

        bytes_read = 0;
        dataLen = 0;
        
        //Azzero il buffer.
        memset(buff, 0 , sizeof(buff));

        while (!close_connection_flag && (bytes_read = read(client_socket, buff + dataLen, sizeof(buff) - dataLen - 1)) >= 0) {
            dataLen += bytes_read;
            //Se esco dai limiti del buffer o leggo un 'a capo' esco dal ciclo.  
            if(bytes_read == 0 || bytes_read == -1 ) close_connection_flag = 1;  // ERRORE
            
            if(buff[dataLen] == '\0' || buff[dataLen - 1] == '\n' || dataLen > (MAXLINE-1)) break;

        }

        if(!close_connection_flag){

            buff[dataLen-1] = 0;

            if((int) bytes_read == -1){ // -- Errore in lettura (Socket da chiudere).
                shutdown(client_socket, SHUT_RDWR);
                perror("Read: ");            
                break;

            } 
            else {    // Lettura avvenuta correttamente. 

                // PARSING DELLA RICHIESTA:

                req = parse_request(buff);

                switch (req->r_op_code) {
                
                    case OP_READ_FILE:

                    break;

                    case OP_WRITE_FILES:    // body: # path + dirname # 
                        
                        size_t letti = 0;
                        path = NULL;
                        dir  = NULL;
                        found = NULL;
                        content = NULL;
                        f_dim = 0;

                        // Leggo dalla richiesta la dimensione del file
                        f_dim = strtol(strtok(req->r_body, "+"), NULL, 10);

                        // Leggo il path (nome del file) :
                        path = strtok(NULL, "+");

                        // E prendo un riferimento alla cartella in cui resituire i file espulsi dalla cache:
                        dir = strtok(NULL, "+");

                        // Controllo l'eventuale contenuto puntato da path: 
                        found = icl_hash_find(hashtable, path);

                        if (found != NULL)  
                        {   
                            // Invia un ACK al client. 
                            if(send_response(client_socket, ACK, INFO_WAITING_FILE) >= 0) {
                                 
                                dataLen = 0;    // quantit?? di bytes ricevuti / buffer
                                
                                memset(buf, 0, BUFSIZE);
                                
                                // Alloco spazio per la chiave e ci copio dentro il path estrapolato da str_token
                                char * key = (char *) calloc(strlen(path) + 1, sizeof(char));
                                strncpy(key, path, strlen(path));
                                        
                                content = calloc(f_dim, sizeof(char));

                                if(content == NULL)
                                {
                                fprintf(stderr, "CONTENT: Memory allocation failed\n");
                                }
                                dir = malloc(sizeof(1024));
                                if(dir == NULL)
                                {
                                fprintf(stderr, "DIR: Memory allocation failed\n");
                                }
                                // TODO: 
                                // Per ora fa una conta dei bytes ricevuti in risposta.
                                while((bytes_read = recv(client_socket, buf, sizeof(buf), 0)) >= 0 && letti < f_dim){
                                    
                                    dataLen += bytes_read;  // Tiene traccia del numero di bytes letti, per controllare se va in overflow.
                                    
                                    fprintf(stderr, "%s", buf);

                                    letti += dataLen;

                                    if(dataLen > (BUFSIZE-1)){
                                        memset(buf, 0, BUFSIZE);
                                        dataLen = 0;
                            
                                    } else if( buf[dataLen] == '\0') break; 
                                }

                                icl_entry_t * test = NULL;

                                if(((test = icl_hash_update_insert(hashtable, key, content, &found)) == NULL))
                                    send_response(client_socket, FILE_NOT_FOUND, INFO_FILE_NOT_FOUND);
                                else 
                                    send_response(client_socket, FILE_WRITTEN, INFO_FILE_WRITTEN);
                                


                            }
                        }

                        else 
                        {

                            fprintf(stderr, "\nRichiesta fallita. Il file non esiste");
                            send_response(client_socket, FILE_NOT_FOUND, INFO_FILE_NOT_FOUND);
                            break;
                        }

                        pthread_cond_signal(&read_cond_var);  

                        // TODO: Inserire contatore da parte del client per il numero di file richiesti: 
                        
                        
                    break;

                    case CLOSE_CONNECTION: 
                    
                        send_response(client_socket, ACK, "Chiusura ordinata avvenuta correttamente. ");
                        close(client_socket);
                        client_socket = -1;
                        close_connection_flag = 1;
                        break;

                    case OP_OPEN_FILE:
                        myfile *    file = malloc(sizeof(myfile));
                        
                        // Inizializzo lista fd (head = NULL) 
                        file->fd_list = NULL;

                        long    o_flag = 0;     // default value
                        path = NULL;
                        found = NULL;
                        
                        
                        // Prendo il primo 'argomento' della richiesta, il path:
                        path = strtok(req->r_body, "+");
                        file->filename = path;
                        file->content = (void *) "we";
                        // Ed il secondo, il flag.
                        o_flag = strtol(strtok(NULL, "+"), NULL, 10);
                        file->flags = o_flag;
                        push(&(file->fd_list), 1, 10);
                        printFile(file);

                        // Controllo l'eventuale contenuto puntato da path: 
                        found = icl_hash_find(hashtable, path);

                        if(found == NULL)   // File non esiste nella hashtable:
                        {
                            if(o_flag & ~O_CREATE){  // ERR: Se non esiste non puoi lavorarci sopra.
                                send_response(client_socket, FILE_NOT_FOUND, INFO_FILE_NOT_FOUND);

                            }
                            else {    // OK -> Crea file.                                
                                
                                // Alloco spazio per la chiave e ci copio dentro il path estrapolato da str_token
                                char * key =  NULL;
                                key = calloc((strlen(path) + 1), sizeof(char));
                                strncpy(key, path, strlen(path));
                                
                                // Inserisco un valore "garbage" nell'entry, per evitare che la icl_hash_find restituisca NULL.
                                icl_entry_t * ret_value = NULL;
                                ret_value = icl_hash_insert(hashtable, key, "x");
                                
                                if(ret_value == NULL)
                                    fprintf(stderr, "\nNon ha proprio funzionato...");

                                if(hashtable != NULL){
                                    fprintf(stderr, "\nDump: \n");
                                    if(icl_hash_dump(stdout, hashtable) == -1) fprintf(stderr, "Non ha");
                                    fprintf(stderr, "\nKey: %s\n", key);

                                } 
                                // Mando risposta al client.
                                send_response(client_socket, FILE_CREATED, INFO_FILE_CREATED);

                            }
                        }
                        else    // File esiste gi??
                        {

                            if(o_flag & O_CREATE)   // ERR: Non puoi creare un file che esiste gi??.
                            {
                                send_response(client_socket, FILE_ALREADY_CREATED, INFO_FILE_ALREADY_CREATED);

                                print_info("OPEN FILE", ERROR, "[Errore] Impossibile creare il file %s, esiste gi??.", path);
                            }
                            else
                            {
                                // Fai la lock. (TODO: ?? giusto?)
                                char * key = calloc((strlen(path) + 1), sizeof(char));
                                strncpy(key, path, strlen(path));

                                if (lock_file(key, req->r_pid, client_socket, hashtable, o_flag) >= 0)
                                {
                                    send_response(client_socket, FILE_LOCKED, INFO_FILE_LOCKED);
                                }
                                else
                                    send_response(client_socket, LOCK_ERROR, INFO_LOCK_ERROR);
                            }
                        }

                        pthread_cond_signal(&read_cond_var);  

                        free(path);
                        break;
                    
                    default:
                        break;
                }
            }
        }
        else 
            break;
    }

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

    memory_size = mem_size;
    files_amount = files_n; 

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

        // Acquisisco la lock.
        Pthread_mutex_lock(&rd_mtx);
        
        //Se non c'?? lavoro da fare mi metto in attesa.
        if((client = dequeue(&read_queue)) == -1){
            
            // E attendo un segnale per essere risvegliato, rilasciando la lock. 
            pthread_cond_wait(&read_cond_var, &rd_mtx);
            
            // Riprovo!
            client = dequeue(&read_queue);
        } 
        // Mollo la lock sulla coda.
        Pthread_mutex_unlock(&rd_mtx);

        // Se il thread ?? stato assegnato ad un task: 
        if(client != -1){
            
            // Passo alla funzione che gestisce i task il client socket.
            * p_client = client;

            // ed eseguo il lavoro.
            connection_handler(p_client);
        }
    }
    
}

void loop_server(){
    
    int client_socket;
    
    while (1)
    {
        client_socket = accept(server_socket, NULL, 0);                                 //-- Si blocca sulla accept.
        fprintf(stderr, "\n[????] Connessione in arrivo sul fd: %d! ", client_socket);    //-- e' arrivato una connessione (si sblocca)!
        
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

/**
 * @brief  Invia al client specificato dal file descriptor c_fd il codice di risposta 
 *         e delle informazioni che possono essere utili in debug. 
 * 
 * @param c_fd file descriptor del client
 * @param op_code 
 * @param response_info  
 * @return ** int (0 successo, codice di errore altrimenti.)
 */
int
send_response(int c_fd, int op_code, char * response_info){
    
    char        buf[BUFSIZE];   

    char        resp[255];
    char        op_str[sizeof(int) + 2];
    
    // Una sorta di Int -> String
    sprintf(op_str, "%d", op_code);
    
    
    /**
     * [OPCODE]#[Response info] 
     * 
     * Segue ^ questo modello per formattare le richieste. 
    */

    strcpy(resp, op_str);
    strcat(resp, "#");
    
    strcat(resp, response_info);
    strcat(resp, "#\n");
    
    /* Richiesta formattata, ora invia al client */ 

    memset(buf, 0, BUFSIZE);
            
    if( write(c_fd, resp, strlen(resp)) < 0) {   // Errore in scrittura
        perror("Write: ");
        return -1;
    }

    return 0; 
}


