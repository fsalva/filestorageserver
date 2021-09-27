#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <string.h>

#include <pthread.h>

#include "./lib/constvalues.h"

#define SERVER_PORT 18081
#define MAXLINE 4096
#define SA struct sockaddr
#define CP configuration_parsed


/* Commentare e decommentare per eseguire parti di codice. */
//#define thread
#define config


//TODO: Sposta su libreria.
    struct configuration_t
    {
        int port_number;
        int worker_thread_number;
        int max_dim;
        int max_files;
        char * bytes_order;

    } configuration_parsed;

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static int x;

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

int main(int argc, char const *argv[])
{
    

    /* Thread */
    pthread_t tid; 
    int err, status;
    
    /* Variabili per la server socket */
    int server_socket, connfd, n;
    struct sockaddr_in servaddr;
    struct sockaddr_in addr;
    socklen_t addr_len;
    char client_address[MAXLINE +1];

    /* Buffer, codificato in utf-8 */
    uint8_t buff[MAXLINE+1];
    uint8_t recvline[MAXLINE+1];
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

     
    if (( server_socket = socket(AF_INET, SOCK_STREAM, 0 )) < 0) //Creo una socket.
        perror("Errore nella creazione della socket: ");
    
    bzero(&servaddr, sizeof(servaddr)); //Azzero l'indirizzo.
    servaddr.sin_family = AF_INET;  
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);   //Accetta ogni tipo di connessione. 
    servaddr.sin_port = htons(SERVER_PORT); //host to network , short ( si occupa della compatibilità big/little endian)

    if((bind(server_socket, (SA *) &servaddr, sizeof(servaddr))) < 0)    //Binding della socket all'indirizzo.
        perror("Errore durante il binding: ");

    if((listen(server_socket, 10)) < 0)  //Si mette in ascolto.
        perror("Errore durante l'ascolto della socket: ");


    //Entro nel loop del server.
    //TODO: condizione d'uscita per spegnere tutto ordinatamente.  
    while(1){
        

        printf("Attendo connessioni sulla porta: %d\nm", SERVER_PORT);
        fflush(stdout);

        //Si blocca in attesa di connessioni, restituisce un file descriptor. 
        connfd = accept(server_socket, (SA *) &addr, &addr_len);

        //network -> presentation
        inet_ntop(AF_INET, &addr, client_address, MAXLINE);

        printf("Client connection: %s\n", client_address);

        memset(recvline, 0 , MAXLINE);  

        while ((n = read(connfd, recvline, MAXLINE-1)) > 0)
        {
            fprintf(stdout, "%s\n", recvline);
            memset(recvline, 0 , MAXLINE);
            
            if(recvline[n-1] == '\n') break; //Esci se ricevi un 'a capo'.

            memset(recvline, 0 , MAXLINE);  //Azzero il buffer per leggere di nuovo.  

            if(n < 0)
                perror("Errore:");

            //Rispondo con un messaggio stupido
            snprintf((char*) buff, sizeof(buff), "HTTP/1.0 200 OK \r\n\r\nHello!");
         
            //Rispondo sulla socket.
            write(connfd, (char*)buff, strlen((char *)buff));
            close(connfd); 
         }

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
                obj->port_number = value; counter++;
                break;
            
            case 1:
                obj->worker_thread_number = value; counter++;
                break;

            case 2:
                obj->max_dim = value; 
                /* Alloco lo spazio che mi serve per dichiarare una stringa che contenga l'ordine
                di grandezza scritto in configurazione. */
                char * make_space = (char *) malloc(sizeof(char) * (strlen(key) + 1));
                strcpy(make_space, key); //E ci copio dentro la stringa
                obj->bytes_order = make_space; counter++;
                break;

            case 3:
                obj->max_files = value; counter++;
                break;

            default:
                break;
            }
        }
    }
    fclose(fptr); 

}
