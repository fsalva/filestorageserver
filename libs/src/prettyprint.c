#include "../prettyprint.h"

#include <unistd.h>
#include <stdio.h>

void clearScreen()
{
  const char *CLEAR_SCREEN_ANSI = "\e[1;1H\e[2J";
  write(STDOUT_FILENO, CLEAR_SCREEN_ANSI, 11);
}


void boxPrint(int handled, int c_socket) {

    printf("%c[2J%c[;H",(char) 27, (char) 27); 
    printf("\n\t ==================================== ");
    printf("\n\t = Inviati %d files.                =", handled);
    printf("\n\t ====================================");
    printf("\n\t = Sto inviando un file al fd: %d   =", c_socket);
    printf("\n\t ====================================");

}