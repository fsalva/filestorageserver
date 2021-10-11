
#include <stdio.h>
#include <stdlib.h>

typedef struct node {
    int val;
    struct node *next;
} node_t;

node_t ** head = NULL;

void enqueue(int * val) {
    node_t *new_node = malloc(sizeof(node_t));
    if (!new_node) return;

    new_node->val = *val;
    new_node->next = *head;

    *head = new_node;
}

int * dequeue() {

    node_t *current, *prev = NULL;
    int * retval = NULL;

    if (*head == NULL) return NULL;

    current = *head;
    
    while (current->next != NULL) {
        prev = current;
        current = current->next;
    }

    retval = &(current->val);
    free(current);
    
    if (prev)
        prev->next = NULL;
    else
        *head = NULL;

    return retval;
}


