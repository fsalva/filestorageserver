#include "../supported_operations.h"
#include "../request.h"
#include "../icl_hash.h"
#include "../const.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>



/**
 * @brief Get the file size object
 * 
 * @param fp 
 * @return size_t 
 */
size_t get_file_size(FILE * fp){
    
    size_t res;

    fseek(fp, 0L, SEEK_END);
    res = ftell(fp);
    rewind(fp);

    return res;
}

/**
 * @brief Legge il contenuto del file fp
 * 
 * @param file 
 * @return void* puntatore al contenuto del file.
 */
void* read_file_content(FILE* fp){
    size_t file_size=get_file_size(fp);

    void* buffer= malloc(sizeof(char) * file_size);
    if(buffer==NULL){
        fprintf(stderr, "[read_file_content]: Impossibile allocare spazio.\n");
        return NULL;
    }
    fread(buffer, sizeof(char), file_size, fp);

    return buffer;
}


int read_file(){
    return 0;
}

/**
 * @brief   Salva un file nella memoria.
 * 
 * @param   path      Percorso (del filesystem) al file da salvare in memoria.
 * @param   t         cache in cui salvare il file. 
 * @param   c_pid     PID del client.
 * @return  ssize_t    quantità di bytes letti o -1 in caso di errore   
 */
size_t write_file(char * path, int c_pid, int c_socket, icl_hash_t * t){

    void *      content = NULL;
    void *      debug = NULL;
    FILE *      fp = NULL;
    size_t      f_size;

    fp = fopen(path, "r");
    if (fp == NULL){
        perror("[write_file, fopen]: ");
        return -1;
    }

    // Inserisco il contenuto del file in content
    content = read_file_content(fp);
    f_size = get_file_size(fp);

    if(icl_hash_insert(t, path, content) == NULL) return -1;


    fclose(fp);
    

    fprintf(stderr, "\nFatto, eh");

    debug = icl_hash_find(t, path);
    
    // ----------- SCRIVE NEL FILE I CONTENUTI DELLA CHIAVE DELL'HASHTABLE
    FILE * file = fopen("/tmp/LIPSUM/fuck.txt", "w");
    fprintf(file, debug);
    
    fclose(file);


    return f_size;
    

}
