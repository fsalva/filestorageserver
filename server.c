#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>

#include <pthread.h>

#define thread
//#define config

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

int main()
{
    /* File di configurazione */
    FILE *fptr;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    /* Thread */
    pthread_t tid; 
    int err, status;


 
    #ifdef config
        if ((fptr = fopen("./config.txt","r")) == NULL){
            //Se non riesco a leggere la configurazione da file, esco.
            perror("[Lettura configurazione]");
            exit(-1);
        }

        while ((read = getline(&line, &len, fptr)) != -1) {
            if(line[0] == '%')
                continue;
            printf("%s", line);
        }
    fclose(fptr); 

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


    return 0;
}