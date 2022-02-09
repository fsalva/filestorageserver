#include "../prettyprint.h"
#include "../constvalues.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>



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

void print_info(char * thisF, int typeOfInfo, char * str, ...){
  	
	switch (typeOfInfo)
	{
	case WARNING:
		printf(ANSI_COLOR_YELLOW "\n[%s]" ANSI_COLOR_RESET, thisF);
		break;
	
	case ERROR:
		printf(ANSI_COLOR_RED "\n[%s]" ANSI_COLOR_RESET, thisF);

		break;

	case SUCCESS:
		printf(ANSI_COLOR_GREEN "\n[%s]" ANSI_COLOR_RESET, thisF);
		break;
	
	case INFO:
		printf(ANSI_COLOR_BLUE "\n[%s]" ANSI_COLOR_RESET, thisF);
		break;
	
	default: break;
	}

	char* s;

    va_list vl;
    va_start(vl, str);


    for (char* ptr = str; *ptr != '\0'; ptr++)
    {
        if (*ptr == '%')
        {
            ptr++;
            s = va_arg(vl, char*);
            while (*s != '\0')
                putchar(*s++);
        }
        putchar(*ptr);
    }
    va_end(vl);

}
