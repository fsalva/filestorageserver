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

#include "./libs/constvalues.h"
#include "./libs/stringutils.h"
#include "./libs/clientapi.h"
#include "./libs/prettyprint.h"

#define BUF_SIZE 4096

int openConnection(const char * sockname, int msec, const struct timespec abst);
int closeConnection(const char * sockname);
void print_usage();

void send_request(int, char, char**);
char * format_request(char*, int, int);
char** str_split(char* a_str, const char a_delim);


int fd_skt, fd_c; 


int main(int argc, char * const argv[])
{
    int         opt;
    int         opcode;
    int         c_pid;

    char *      socket_n; //-- path alla server socket
    char **     arguments;

    c_pid = getpid();
    

    const struct timespec x = {5, 0};

    while ((opt = getopt(argc, argv, "hf:w:WDr:R:dtlucp")) != -1) 
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
        
        case 'w':
            arguments = str_split(optarg, (char) ',');
            opcode = OP_WRITE_FILES;
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

    if( (fd_skt = (openConnection(socket_n, 100, x))) < 0){
        print_debug("Errore durante connessione con il server. Timeout.\n", 1);
    }
    else {
        fprintf(stderr, "\nConnesso alla socket: %d", fd_skt);
        fprintf(stderr, "\n[OPCODE]: %d, [ARGUMENTS]: %s\n", opcode, arguments[0]);

        send_request(c_pid, opcode, arguments);
        
        close(fd_skt);

        exit(EXIT_SUCCESS);

    }
    
    exit(EXIT_FAILURE);

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
    
    return fn;
}

void send_request(int pid, char opt, char ** arguments){

    int         i;
    int         letti;
    char *      request = NULL;
    char        buf[BUF_SIZE];

    size_t bytes_read;
    size_t dataLen;

    if(arguments){

        // Per ogni argomento della richiesta: 
        for (i = 0; *(arguments + i); i++)
        {   
            // Formattalo in una singola richiesta: 
            request = format_request(*(arguments + i), opt, pid);
            
            print_debug(request, 1);

            letti = 0;  // quantita' di bytes ricevuti / richiesta.
            
            memset(buf, 0, BUF_SIZE);
            
            // Ed invialo sulla socket.
            write(fd_skt, request, strlen(request));

            dataLen = 0;    // quantità di bytes ricevuti / buffer
            
            // TODO: 
            // Per ora fa una conta dei bytes ricevuti in risposta.
            while((bytes_read = (recv(fd_skt, buf, sizeof(buf), 0)) >= 0)){
            
                dataLen += bytes_read;
                
                if(dataLen > (BUF_SIZE-1)){
                    letti += dataLen;
                    memset(buf, 0, BUF_SIZE);
                    dataLen = 0;
                    
                } else if( buf[dataLen] == '\000') break; 
            }

            letti += dataLen;

            printf("[PID: %d][+] RECEIVED: %d bytes.\n",getpid(), letti);

            free(*(arguments + i));
        }

        printf("\n");
        free(arguments);
    }

   
}

void print_usage(const char * argv[]){

    printf("usage:  %s  -option [-option ...]\n"
       "Lista delle opzioni:\n"
       "    -h                  :   Stampa questo messaggio.\n"
       "    -f filename         :   Specifica il nome della socket a cui connettersi.\n"
       "    -w dirname[,n=0]    :   Invia al server n file contenuti nella cartella 'dirname',\n"
       "                        :   se n = 0, li invia tutti.\n"
       "    -W file_1[,file_2]  :   Invia al server i file per essere scritti.\n"
       "    -D dirname          :   Specifica della cartella lato client su cui salvare i file\n"
       "                        :   espulsi dal server. [Richiede opzioni -w o -W]\n"
       "    -r file_1[,file_2]  :   Lista di file da leggere dal server.\n"
       "    -R [n=0]            :   Legge n file qualsiasi dal server; Se n = 0 li legge tutti.\n"
       "    -d dirname          :   Specifica della cartella dove salvare i file letti dal server. \n"
       "    -t time             :   Tempo [msec] tra una richiesta e la successiva.\n"
       "    -l file_1[,file_2]  :   Lista di nomi di file su cui acquisire mutex. \n"
       "    -u file_1[,file_2]  :   Lista di nomi di file su cui rilascaire mutex. \n"
       "    -c file_1[,file_2]  :   Lista di nomi di file da rimuovere dal server. \n"
       "    -p                  :   Abilita le stampe su stdout per ogni operazione.\n"
       , argv[0]);
}


char** str_split(char* a_str, const char a_delim)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}