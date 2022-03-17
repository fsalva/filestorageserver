#include "../myfile.h"
#include "../list.h"

#include <stdio.h>

void
printFile(myfile * f){
    fprintf(stderr, "\n");
    fprintf(stderr, "[%s]\n", f->filename);
    fprintf(stderr, "Contenuto: \n%s\n", (char *) f->content);
    fprintf(stderr, "Flag: %d\n", f->flags);
    fprintf(stderr, "Lista di FD:\n ");
    printList(f->fd_list);

}
