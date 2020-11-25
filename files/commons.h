#include "error.h"
#define SOCKET_NAME "./objstore.sock"


char *strdup(const char *s);

/* invia messaggio all'interlocutore */
int writeTo(char *msg, int fd);

/* returna il messaggio dell'interlocutore */
char *readFrom(int fd);

/* per una corretta scrittura tra client e server */
int readn(long fd, void *buf, size_t size);

/* per una corretta lettura tra client e server */
int writen(long fd, void *buf, size_t size);

/*  tokenizza la stringa passata sul socket
    e controlla con check_args() che sia una richiesta valida */
char **tokenizer(char *msg);

/* dealloca le stringhe di args */
void destroyBuff(char **args);

/* creo il path desiderato */
char *mk_path(char *name, ...);