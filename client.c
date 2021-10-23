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
#include "./lib/fsstring.h"
#include "./lib/constvalues.h"

#define BUF_SIZE 4096

int openConnection(const char * sockname, int msec, const struct timespec abst);
int closeConnection(const char * sockname);
void print_usage();

void send_request(int, char, char**);
char * format_request(char*, int, int);
char** str_split(char* a_str, const char a_delim);


int fd_skt, fd_c; 

int main(int argc, char const *argv[])
{
    int flags, opt;
    int nsecs, tfnd;

    char * req  = "/tmp/TESTFILES/lipsum15.txt\n";
    char * req2 = "/tmp/TESTFILES/lipsum1.txt\n";
    char * avalue = NULL;
    
    char *socket_n; //-- path alla server socket
    char ** arguments;
    int opcode;

    int c_pid = getpid();

    nsecs = 0;
    tfnd = 0;
    flags = 0;

    const struct timespec x = {1, 0};


    while ((opt = getopt(argc, argv, "hf:wWDr:R:dtlucp")) != -1) 
    {
        switch (opt) 
        {
        case 'f':
            socket_n = optarg;
            break;

        case 'r':
            arguments = str_split(optarg, (char) ',');
            opcode = OP_READ_FILE;
            break;
        
        case 'h':
            print_usage(argv);
            exit(EXIT_SUCCESS);
        break;

        case '?':
            print_usage(argv);
            exit(EXIT_FAILURE);
        break;


        }
    }

    openConnection(socket_n, 100, x);

    send_request(c_pid, opcode, arguments);

    

    
    //closeConnection();    
    
    close(fd_skt);

    exit(EXIT_SUCCESS);

}
    


int openConnection(const char * sockname, int msec, const struct timespec abst){

    //Nome socket. --> "./socket/l.sock"
    char * socketName = sockname;

    struct sockaddr_un sa;

    strncpy(sa.sun_path, socketName, 108);
    sa.sun_family=AF_UNIX;

    fd_skt = socket(AF_UNIX,SOCK_STREAM,0);

    if(connect(fd_skt, (struct sockaddr *) &sa, sizeof(sa)) == -1){
            perror("[-] Errore in connessione: ");
            exit(EXIT_FAILURE);
    }   else fprintf(stderr, "[+] Connesso.\n");
    
    return 0;
}

int closeConnection(const char * sockname){
    close(fd_skt);
    return 0;
}

char * format_request(char* request_body, int opt, int pid){

    char * fn = (char *) malloc(sizeof(char) * 255);
    char op_str[sizeof(int) + 2];
    sprintf(op_str, "%d", opt);
    
    fprintf(stderr, op_str);

    char pid_str[sizeof(int) + 2];
    sprintf(pid_str, "%d", pid);

    /**
     * [OPCODE]#[pid]#[request body] 
     * 
     * Segue ^ questo modello per formattare le richieste. 
    */
    strcpy(fn, op_str);
    strcat(fn, "#");

    strcat(fn, pid_str);
    strcat(fn, "#");

    strcat(fn, request_body);
    strcat(fn, "#\n");

    

    fprintf(stderr, fn);
    
    return fn;
}

void send_request(int pid, char opt, char ** arguments){
    
    if(arguments){
        int i;
        char * request = NULL;

        for (i = 0; *(arguments + i); i++)
        {
            request = format_request(*(arguments + i), opt, pid);

            //Invia la richiesta. 
             int letti = 0;

            char buf[BUF_SIZE];
            
            memset(buf, 0, BUF_SIZE);
            
            //invia sulla socket.
            write(fd_skt, request, strlen(request));

            size_t bytes_read;
            size_t dataLen = 0;
                
            while((bytes_read = (recv(fd_skt, buf, sizeof(buf), 0))) >= 0){

                dataLen += bytes_read;
                
                if(dataLen > (BUF_SIZE-1)){
                    letti += dataLen;
                    memset(buf, 0, BUF_SIZE);
                    dataLen = 0;
                    
                } else if( buf[dataLen] == '\000') break; 
            }

            letti += dataLen;

            printf("[PID: %d][+] RECEIVED: %d bytes.\n",getpid(), letti);


            //------------------

            free(*(arguments + i));
        }

        printf("\n");
        free(arguments);
    }

   
}

void print_usage(const char * argv[]){

    printf("usage:  %s  -option [-option ...]\n"
       "Lista delle opzioni:\n"
       "    -h                  :   Stampa questo messaggio.\n\n"
       "    -f filename         :   Specifica il nome della socket a cui connettersi.\n\n"
       "    -w dirname[,n=0]    :   Invia al server n file contenuti nella cartella 'dirname',\n"
       "                        :   se n = 0, li invia tutti.\n\n"
       "    -W file_1[,file_2]  :   Invia al server i file per essere scritti.\n\n"
       "    -D dirname          :   Specifica della cartella lato client su cui salvare i file\n"
       "                        :   espulsi dal server. [Richiede opzioni -w o -W]\n\n"
       "    -r file_1[,file_2]  :   Lista di file da leggere dal server.\n\n"
       "    -R [n=0]            :   Legge n file qualsiasi dal server; Se n = 0 li legge tutti.\n\n"
       "    -d dirname          :   Specifica della cartella dove salvare i file letti dal server. \n"
       "                        :   espulsi dal server. [Richiede opzioni -r o -R]\n\n"
       "    -t time             :   Tempo [msec] tra una richiesta e la successiva.\n\n"
       "    -l file_1[,file_2]  :   Lista di nomi di file su cui acquisire mutex. \n\n"
       "    -u file_1[,file_2]  :   Lista di nomi di file su cui rilascaire mutex. \n\n"
       "    -c file_1[,file_2]  :   Lista di nomi di file da rimuovere dal server. \n\n"
       "    -p                  :   Abilita le stampe su stdout per ogni operazione.\n\n"
       , argv[0]);
}

