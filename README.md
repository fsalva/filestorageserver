# filestorageserver
memory-based file storage server - course project for LSO @UniPi

## Configurazione
La configurazione viene letta dal file config.txt. 
I commenti sono individuati dal carattere '%'. 
La configurazione serve per impostare alcune caratteristiche importanti, come la dimensione della memoria, il numero thread worker, ecc..

## Operazioni
Il client invia al server una richiesta formattata così: 
[codice_operazione]#[pid_client]#[argomenti]
La richiesta viene processata dal server che risponde con un codice.
[codice_errore/codice_successo](# [corpo_risposta] ) 