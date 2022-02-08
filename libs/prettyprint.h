#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define WARNING	0
#define SUCCESS 1
#define ERROR	2
#define INFO	3

void clearScreen();

void boxPrint(int, int);

void print_debug(char * text, int), 
    print_info(char *, int, char *, ...);