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

#define CLOSE_CONNECTION    999
#define ACK                 200
#define FILE_CREATED        204
#define FILE_LOCKED         205
#define FILE_UNLOCKED       206
#define FILE_WRITTEN        207

#define INFO_FILE_CREATED   "File creato correttamente."
#define INFO_FILE_LOCKED    "Lock acquisita sul file."
#define INFO_FILE_UNLOCKED  "Lock rilasciata dal file."
#define INFO_FILE_WRITTEN   "Contenuto del file scritto in memoria."

#define FILE_NOT_FOUND          404
#define FILE_ALREADY_CREATED    405
#define LOCK_ERROR              406
#define FILE_NOT_CREATED        504

#define INFO_FILE_NOT_FOUND         "File inesistente."
#define INFO_FILE_NOT_CREATED       "Impossibile creare il file specificato."
#define INFO_FILE_ALREADY_CREATED   "File gia' presente in memoria."
#define INFO_LOCK_ERROR             "Errore nel prendere la lock."
#define INFO_WAITING_FILE           "In attesa di ricevere i contenuti del file."

typedef int make_iso_compilers_happy;