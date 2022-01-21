/**
 * @file request.c
 * @author Francesco Salvatori
 * @brief Funzioni ausiliarie per fare il parsing delle richieste da parte del client.
 * @version 0.1
 * @date 2022-01-21
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "../request.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/**
 * @brief parsing delle richieste di lettura di un file.
 * 
 * @param request_body
 * @return ** struct request parsed* 
 */
request * parse_request(char * request_body){
        
    // Alloco spazio per la struct di ritorno
    request * result = malloc(sizeof(result));

    // Alloco spazio per la stringa
    char * s = calloc(strlen(request_body), sizeof(char));

    strcpy(s, request_body);

    char* token = strtok(s, "#");
    

    int count = 0;

    while (token) {

        switch (count)
        {
        case 0:
            result->r_op_code = strtol(token, NULL, 10);    
            break;
        
        case 1:
            result->r_pid = strtol(token, NULL, 10);
            break;

        case 2:
            result->r_path = malloc(strlen(token) * sizeof(char));
            strcpy(result->r_path, token); 
            break;

        default:
            break;
        }    

        token = strtok(NULL, "#");
        count ++;
    }

    free(s);

    return result;
}
