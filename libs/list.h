#ifndef _MY_LIST_H_
#define _MY_LIST_H_

#include <stdbool.h>

typedef struct elem {
   int key;
   int data;
   struct elem *next;
} elem_t;

/**
 * @brief Inserisce un elemento in testa alla lista.
 * 
 * @param head puntatore alla lista.
 * @param key  chiave da inserire
 * @param data dato da inserire
 */
void
push (elem_t ** head, int key, int data);

/**
 * @brief Elimina l'elemento in testa alla lista.
 * 
 * @param head puntatore alla lista
 * @return elem_t* dato eliminato
 */
elem_t *
pop (elem_t ** head);

/**
 * @brief Stampa la lista
 * 
 * @param head 
 */
void
printList(elem_t * head);


/**
 * @brief Restituisce vero se la lista è vuota.
 * 
 * @param head 
 * @return true 
 * @return false 
 */
bool 
isEmpty(elem_t *head);

/**
 * @brief Restituisce la lunghezza della lista
 * 
 * @param head 
 * @return int 
 */
int 
length (elem_t *head);

/**
 * @brief Restituisce l'elemento corrispondente alla chiave cercata o null se non trovato.
 * 
 * @param head 
 * @param key 
 * @return elem_t* 
 */
elem_t * 
find (elem_t * head, int key);


/**
 * @brief Elimina un elemento specifico a partire dalla chiave.
 * 
 * @param head 
 * @param key 
 * @return elem_t* 
 */
elem_t * 
delete(elem_t ** head, int key);



#endif
