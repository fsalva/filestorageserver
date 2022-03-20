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
#include "./libs/myfile.h"
#include "./libs/list.h"

#define FILENAME "/home/francesco/Documents/Unipi/LSO/filestorageserver/test.txt"

void print_usage();

int fd_c; 


int 
main(int argc, char * const argv[])
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
    else {  // TEST DEBUG

        openFile(FILENAME, O_CREATE);
        openFile(FILENAME, O_LOCK);
        
        writeFile(FILENAME, "/test");

        if (closeConnection(socket_n) == 0)  print_debug("Chiudo! \n", 1);
        else
            print_debug("Fallito! \n", 1);

        exit(EXIT_SUCCESS);

    }

    
    exit(EXIT_FAILURE);

}

void 
print_usage(const char * argv[]){

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


