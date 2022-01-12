/**
 * @file config_parser.h
 * @author Francesco Salvatori (francesco@salvatori.dev)
 * @brief Interfaccia per la libreria del parser della configurazione.
 * @version 0.1
 * @date 2022-01-12
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#define CONFIG_PATH = "./config/config.txt"

struct config_parser
{
     int max_dim;
     int max_file_n;
     int thread_workers_n;
     char magnitude[2];
     char * socket_path;
};
typedef struct config_parser config_parser;

extern int parse(config_parser *);
extern void config_cleanup(config_parser * );

#endif