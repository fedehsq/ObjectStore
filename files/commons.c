#define _POSIX_C_SOURCE  200112L
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include "error.h"
#include "commons.h"


/* invia risposta all'interlocutore */
int writeTo(char *msg, int fd) {
    int size = strlen(msg) + 1;
    int n = writen(fd, &size, sizeof(int));
    /* il server ha ricevuto un segnale e ha smesso di accettare connessioni */
    if (n == -1) {
        return -1;
    }

    n = writen(fd, msg, size * sizeof(char));
    /* il server ha ricevuto un segnale e ha smesso di accettare connessioni */
    if (n == -1) {
        return -1;
    }

    /* scrittura completata correttamente */
    return 1;
}


/* returna il messaggio dell'interlocutore */
char *readFrom(int fd) {
    char *buf;
    int size;
    int n = readn(fd, &size, sizeof(int));

    /* il server ha ricevuto un segnale e ha smesso di accettare connessioni */
    if (n == -1) {
        return NULL;
    }

    /* server chiuso */
    if (n == 0) {
        return NULL;
    } 

    EC_NULL(buf = (char*)calloc(size, sizeof(char)), "calloc");
    
    n = readn(fd, buf, size * sizeof(char));
    
    /* il server ha ricevuto un segnale e ha smesso di accettare connessioni */
    if (n == -1) {    
        return NULL;
    }

    /* server chiuso */
    if (n == 0) {
        free(buf);
        return NULL;
    } 

    /* messaggio salvato correttamente */
    return buf;
}


int readn(long fd, void *buf, size_t size) {
    size_t left = size;
    int r;
    char *bufptr = (char*)buf;
    while (left > 0) {
	    if ((r = read((int)fd, bufptr, left)) == -1) {
            /* interrupted system call */
	        if (errno == EINTR) {
                continue;
            }
            return -1;
	    }
        /* socket chiuso */
	    if (r == 0) { 
            return 0;
        } 
        left -= r;
	    bufptr += r;
    }
    return size;
}


int writen(long fd, void *buf, size_t size) {
    size_t left = size;
    int r;
    char *bufptr = (char*)buf;
    while (left > 0) {
        if ((r = write((int)fd, bufptr, left)) == -1) {
            /* interrupted system call */
            if (errno == EINTR) {
                continue;

            } else {
                return -1;
            }
        }
        if (r == 0) {
            return 0; 
        }
        left -= r;
        bufptr += r;
    }
    return 1;
}

/*  tokenizza la stringa passata sul socket */
char **tokenizer(char *msg) {
    /* msg non valido */
    if (msg == NULL) {
        return NULL;
    }
    /* ogni parola di msg diventa una stringa di **splitted_msg */
    int i = 1;
    char **splitted_msg = NULL;

    char *token, *save_ptr;
    token = strtok_r(msg, " \n", &save_ptr);
    while (token) {
        /* atto a contenere una parola in più */
        splitted_msg = (char**)realloc(splitted_msg, (++i) * sizeof(char *));
        /* copio la parola */
        splitted_msg[i - 2] = strdup(token);

        token = strtok_r(NULL, " \n", &save_ptr);
    }
    /* terminatore per stringhe */
    splitted_msg[i - 1] = NULL;
    return splitted_msg;
}


/* dealloca le stringhe di args */
void destroyBuff(char **args) {
    char **p = args;
    while (*p) {
        free(*p);
        p = p + 1;
    }
}


/* creo il path desiderato */
char *mk_path(char *name, ...) {
    
    /* +3 per "./" */
    int sz = strlen(name) + 3;

    char *tmp, *path;
    EC_NULL(path = (char *)calloc(sz, sizeof(char)), "calloc"); 
    snprintf(path, sz, "%s", name);
    va_list va;
    va_start(va, name);
    while ((tmp = va_arg(va, char *))) {
        sz += strlen(tmp) + 2;
        EC_NULL(path =(char*)realloc(path, sz), "realloc");
        strcat(path, "/");
        strncat(path, tmp, sz);
    }
    va_end(va);
    return path;
}