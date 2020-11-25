#include <stdio.h>

/* file desfriptor del socket */
int fd_socket;

/* Registra il cliente con il name dato.
Restituisce true se la connessione ha avuto successo, false altrimenti */
int os_register(char *name);

/*  inizia la connessione all'object store, 
    se l'utente è registrato */
int os_connect(char *name);

/*  chiede all'object store la memorizzazione
    dell'oggetto puntato da block, per una lunghezza len, con il nome name. 
    Restituisce true se la memorizzazione ha avuto successo,
     false altrimenti. */
int os_store(char *name, void *block, size_t len);

/*  recupera dall'object store l'oggetto precedentementememorizzatato sotto il nome name.
    Se il recupero ha avuto successo, restituisce un puntatore a un
    blocco di memoria, allocato dalla funzione, contenente i dati precedentemente memorizzati. 
    In caso di errore, restituisce NULL */

void *os_retrieve(char *name);

/*  cancella l'oggetto di nome name precedentemente memorizzato.
    Restituisce true se la cancellazione ha avuto successo, false altrimenti. */
int os_delete(char *name);

/* chiude la connessione all'object store. Restituisce true se la disconnessione ha
avuto successo, false in caso contrario. */
int os_disconnect();
