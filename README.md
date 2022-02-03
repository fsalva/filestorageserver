# filestorageserver
memory-based file storage server - course project for LSO @UniPi

## Configurazione
La configurazione viene letta dal file config.txt. <br>
I commenti sono individuati dal carattere '%', e vi è la possibilità di realizzare commenti multi-linea.  <br>
La configurazione serve per impostare alcune caratteristiche importanti, come la dimensione della memoria, il numero thread worker, ecc..

## Operazioni
Per ogni operazione da riga di comando, il client invia al server una richiesta formattata: <br> <code> 
[codice_operazione]#[pid_client]#[arg₁ + ... + argₙ]</code> <br> 
La richiesta viene parsata e processata dal server che risponde con un codice. <br><code>
[codice_errore/codice_successo]#[corpo_risposta]</code> 
<br> <br>
Per esempio, la richiesta: <br>  `-w file₁, ... , fileₙ -r fileₓ -d dir` <br>
Corrisponderà a <b> due richieste</b>, una di scrittura di n file, ed una di lettura di un file. 