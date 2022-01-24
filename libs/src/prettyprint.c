#include "../prettyprint.h"
#include "../constvalues.h"

#include <unistd.h>
#include <stdio.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

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

void print_debug(char * text, int debug_flag){
	
	if(debug_flag)
	{
		fprintf(stderr, ANSI_COLOR_YELLOW "[!] %s" ANSI_COLOR_RESET, text);
	}
}