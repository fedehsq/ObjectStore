#define _POSIX_C_SOURCE  200112L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "commons.h"
#include "error.h"
#include "requests.h"

char *strdup(const char *s);

int fd_socket;

/* invia la richiesta al server e returna la risposta */
char *writeTo_readFrom(char *request, int fd) {

    /* server chiuso tramite segnale  */
    if (writeTo(request, fd_socket) == -1) {
        return NULL;
    }
    return readFrom(fd_socket);
}

/* returna true se la risposta del server è "OK" */
bool checkAnsw(char *answer) {
    
    /* server chiuso tramite segnale */
    if ((answer == NULL)) {
        return false;
    }

    /* op conclusa con successo */
    if (strcmp("OK", answer) == 0) {
        free(answer);
        return true;
    }

    /* op conclusa con falliemnto */
    free(answer);
    return false;
}

/* Registra il cliente con il name dato.
Restituisce true se la connessione ha avuto successo, false altrimenti */
int os_register(char *name) {

    /*  14 per "REGISTER %s\n" */
    int size = 14 + strlen(name); 
    char *msg, *answer;
    EC_NULL(msg = (char*)calloc(size, sizeof(char)), "calloc");
    snprintf(msg, size, "REGISTER %s\n", name);

    /* scrivo/leggo sul/dal socket */
    answer = writeTo_readFrom(msg, fd_socket);
    free(msg);

    /* esito op */
    return checkAnsw(answer);
}

/*  inizia la connessione all'object store, 
    se l'utente è registrato */
int os_connect(char *name) {
    int size = 13 + strlen(name); //13 per "CONNECT %s\n"
    char *msg, *answer;
    EC_NULL(msg = (char*)calloc(size, sizeof(char)), "calloc");
    snprintf(msg, size, "CONNECT %s\n", name);

    /* scrivo/leggo sul/dal socket */
    answer = writeTo_readFrom(msg, fd_socket);
    free(msg);

    /* esito op */
    return checkAnsw(answer);
    

}

/*  chiede all'object store la memorizzazione
    dell'oggetto puntato da block, per una lunghezza len, con il nome name. 
    Restituisce true se la memorizzazione ha avuto successo,
     false altrimenti. */
int os_store(char *name, void *block, size_t len) {
    int size = 17 + strlen(name) + len; //17 per "STORE %s %ld\n%s"
    char *msg, *answer;
    EC_NULL(msg = (char*)calloc(size, sizeof(char)), "calloc");
    snprintf(msg, size, "STORE %s %ld\n%s", name, len, (char *)block);

    /* scrivo/leggo sul/dal socket */
    answer = writeTo_readFrom(msg, fd_socket);
    free(msg);

    /* esito op */
    return checkAnsw(answer);
}

/*  recupera dall'object store l'oggetto precedentementememorizzatato sotto il nome name.
    Se il recupero ha avuto successo, restituisce un puntatore a un
    blocco di memoria, allocato dalla funzione, contenente i dati precedentemente memorizzati. 
    In caso di errore, restituisce NULL */
void *os_retrieve(char *name) {

    /* blocco da restituire */
    void *data = NULL;

    /* lunghezza del blocco */
    size_t len;
    char *end_ptr;

    /* 14 per "RETIREVE %s\n" */
    int size = 14 + strlen(name);

    /* richiesta da inviare */ 
    char *msg, *answer;
    EC_NULL(msg = (char*)calloc(size, sizeof(char)), "calloc");
    snprintf(msg, size, "RETRIEVE %s\n", name);

    /* invio richiesta al server e salvo risposta del server */
    answer = writeTo_readFrom(msg, fd_socket);
    if (answer == NULL) {
        free(msg);
        return data;
    }
    
    /* splitto la risposta  */
    char **splitted_answer = tokenizer(answer);
    
    /* se come secondo arg ho la size il retrieve ha returnato un blocco, 
       altrimenti avrò il messaggio di errore */
    len = strtol(splitted_answer[1], &end_ptr, 10);
    if (*end_ptr != (char)0) {
        goto exit;
    }  

    EC_NULL(data = (char*)calloc(len, sizeof(char)), "calloc"); 
    strncpy(data, splitted_answer[2], len);
    exit:
    destroyBuff(splitted_answer);
    free(splitted_answer);
    free(answer);
    free(msg);
    return data;
}

/*  cancella l'oggetto di nome name precedentemente memorizzato.
    Restituisce true se la cancellazione ha avuto successo, false altrimenti. */
int os_delete(char *name) {
    int size = 12 + strlen(name); //12 per "DELETE "
    char *msg, *answer;
    EC_NULL(msg = (char*)calloc(size, sizeof(char)), "calloc");
    snprintf(msg, size, "DELETE %s\n", name);
   
    /* scrivo/leggo sul/dal socket */
    answer = writeTo_readFrom(msg, fd_socket);
    free(msg);

    /* esito op */
    return checkAnsw(answer);
}

/* chiude la connessione all'object store. Restituisce true se la disconnessione ha
avuto successo, false in caso contrario. */
int os_disconnect() {
    int size = 8; //8 per "LEAVE\n"
    char *msg, *answer;
    CHAR_MALLOC(msg, size);
    snprintf(msg, size, "LEAVE\n");

    /* scrivo/leggo sul/dal socket */
    answer = writeTo_readFrom(msg, fd_socket);
    free(msg);

    /* esito op */
    return checkAnsw(answer);
}