typedef struct node {
    int val;
    struct node *next;
} node_t;


void enqueue(node_t **, int);

int * dequeue(node_t **);
