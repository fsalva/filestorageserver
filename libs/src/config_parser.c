#define  _GNU_SOURCE

#include "../config_parser.h"

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

void remove_spaces(char* s);

/**
 * @param cp puntatore alla struct in cui salvare le informazioni di config. 
 * @return 1 se tutto ok, errore <= 0 se incontra errori.  
 */
int parse(config_parser * cp){

    //  fptr -> config.txt
    FILE * fptr;

    char * line = NULL; // -> righe nel file
    size_t len = 0;     // lunghezza di ogni riga
    size_t l_read;     // linee lette
    
    //  chiave : valore
    char * key = NULL;
    char * value = NULL;
    char * mag = NULL;

    if ((fptr = fopen("./config/config.txt","r")) == NULL){
        perror("[config_parser.c]");
        return -1;
    }

    // legge riga per riga il file
    while ((l_read = getline(&line, &len, fptr)) != -1)
    {
        if (line[0] == '%' ||   // è un commento
            strcmp(line, "\n") == 0 || strcmp(line, "\r\n") == 0) // è uno spazio
            continue;
        else {   
        
            // prende la substring prima del '='
            key = strtok(line, "="); 
            remove_spaces(key);
            // ...e quella successiva
            if(key != NULL) value = strtok(NULL, "=");
            
            remove_spaces(value);
            if (strcmp(key, "THREAD_WORKERS") == 0) cp->thread_workers_n = strtol(value, &mag, 10); 
            if (strcmp(key, "MAX_DIM") == 0)  {cp->max_dim = strtol(value, &mag, 10); strncpy(cp->magnitude, mag, strlen(mag));}
            if (strcmp(key, "MAX_FILES") == 0) cp->max_file_n = strtol(value, &mag, 10);
            if (strcmp(key, "SOCKET_NAME") == 0) {
                // alloca spazio per il path:
                cp->socket_path = malloc(sizeof(char) * strlen(value));
                strncpy(cp->socket_path, value, strlen(value));        
            }
        }
    }
    
    #define prettyprint
    #ifdef prettyprint
     fprintf(stderr, 
        "[Info]\n\tPath della socket: %s\tThread workers:  %d\n\tNumero massimo di file memorizzabili: %d\n\tSpazio disponibile: %d%s", 
        cp->socket_path, 
        cp->thread_workers_n, 
        cp->max_file_n, 
        cp->max_dim, 
        cp->magnitude);
    #endif
   

    free(line);
    fclose(fptr);

    // tutto corretto, esco.
    return 1;
}

/**
 * @brief Libera la memoria, da chiamare alla chiusura del server.
 * @param cp puntatore alla struct da 'pulire'
 */
void config_cleanup(config_parser * cp){
    if(cp != NULL){
        free(cp->socket_path);
        free(cp);
    }
}

/**
 * @brief Funzione ausiliaria per evitare problemi dovuti a spaziature nel file di configurazione.
 * 
 * @param s la stringa da modificare.
 */
void remove_spaces(char* s) {
    char* d = s;
    do {
        while (*d == ' ') {
            ++d;
        }
    } while ((*s++ = *d++) != 0);
}