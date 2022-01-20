/**
 * @file server.h
 * @author Francesco Salvatori (francesco@salvatori.dev)
 * @brief Libreria per le funzioni di 'mantenimento' del server:
 *        Gestione dei thread,
 *        Gestione dei task,
 *        Mutex (...) 
 * @version 0.1
 * @date 2022-01-12
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef SERVER_H
#define SERVER_H

#define SA struct sockaddr_un

#include<pthread.h>
#include<sys/types.h>

extern int server_socket; // fd su cui si connettono i client.
extern int handledSuccessfully;

extern void * connection_handler(void *);
extern void * thread_funtion(void *);
extern void * monitor_function(void *);

extern void * interruptHandler();

extern int start_server(int, int, int, char *);
extern int stop_server();
extern void loop_server();
extern void cleanup();

void Pthread_mutex_lock(pthread_mutex_t *);
void Pthread_mutex_unlock(pthread_mutex_t *);

#endif