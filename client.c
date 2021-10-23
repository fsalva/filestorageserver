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

#define BUF_SIZE 4096

int openConnection(const char * sockname, int msec, const struct timespec abst);
int closeConnection(const char * sockname);
void print_usage();

void send_request(char *, char);
char * format_request(char*, int);
char** str_split(char* a_str, const char a_delim);


int fd_skt, fd_c; 

int main(int argc, char const *argv[])
{
    int flags, opt;
    int nsecs, tfnd;

    char * req  = "/tmp/TESTFILES/lipsum15.txt\n";
    char * req2 = "/tmp/TESTFILES/lipsum1.txt\n";
    char * avalue = NULL;
    char ** arguments;
    
    char *socket_n; //-- path alla server socket
    
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

        case 'R':
            //strcpy(avalue, optarg);
            fprintf(stderr, optarg);
            arguments = str_split(optarg, (char) ',');

            //avalue = format_request(optarg, opt);
            //openConnection(NULL, 0, x);
            //send_request(avalue, opt);

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

    if(arguments){
        int i;
        
        for (i = 0; *(arguments + i); i++)
        {
            printf("\nfiles=[%s]\n", *(arguments + i));

            strcat(*(arguments + i), "\n");

            send_request(*(arguments + i), opt);

            free(*(arguments + i));
        }

        printf("\n");
        free(arguments);
    }

    
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
    }   else fprintf(stderr, "\n[+] Connesso.");
    
    return 0;
}

int closeConnection(const char * sockname){
    close(fd_skt);
    return 0;
}

char * format_request(char* request_body, int opt){



    char * fn = (char *) malloc(sizeof(char) * 255);
    char str[2] = {opt, '\0'};

    strcpy(fn, str);
    strcat(fn, "#");

    strcat(fn, request_body);
    strcat(fn, "#");


    fprintf(stderr, fn);
    
    return fn;
}

void send_request(char * request_body, char opt){
    
    int letti = 0;

    char buf[BUF_SIZE];
    
    memset(buf, 0, BUF_SIZE);

    //manda richiesta
    write(fd_skt, request_body, strlen(request_body));

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