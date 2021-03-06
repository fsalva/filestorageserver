
//Costanti
#define MAXLINE 4096
#define BUFSIZE 4096
#define UNIX_PATH_MAX 108
#define SERVER_QUEUE 100
#define THREAD_POOL_SIZE 20

//Operazioni
#define OP_OPEN_FILE    101
#define OP_READ_FILE    102
#define OP_READ_N_FILES 103
#define OP_WRITE_FILES  104
#define OP_APPEND_FILE  105
#define OP_LOCK_FILE    106
#define OP_UNLOCK_FILE  107
#define OP_CLOSE_FILE   108
#define OP_REMOVE_FILE  109

enum _FILE_FLAGS { 
   O_CREATE = 0x1,
   O_LOCK   = 0x2, 
   O_READ   = 0x4,
   O_WRITE  = 0x8
};

typedef int make_iso_compilers_happy;
