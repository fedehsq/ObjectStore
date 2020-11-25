/* Il client di test deve esercitare tutte le funzioni offerte dalla libreria, con cui dovrà essere linkato
staticamente. A tale scopo, dovrà essere in grado, in esecuzioni distinte, di
1. creare e memorizzare oggetti. Il client dovrà generare 20 oggetti, di dimensioni crescenti da 100 byte
a 100000 byte, memorizzandoli sull'object store con nomi convenzionali. Gli oggetti dovranno
contenere dati “artificiali” facilmente verificabili (per esempio: byte di valore numerico consecutivo
0, 1, 2... oppure sequenze di 8 byte di valore 8, poi 9 byte di valore 9, 10 byte di valore 10 ecc.,
oppure più copie di una stringa di testo prefissata, ecc.)
2. di recuperare oggetti verificando che i contenuti siano corretti
3. di cancellare oggetti 

Il client riceverà come argomento sulla riga di comando il nome cliente da utilizzare nella connessione con
l'object store, e un numero nell'intervallo 1-3 corrispondente alla batteria di test da effettuare (come indicato
sopra). Terminati i test della batteria richiesta, il client deve uscire, e stampare sul suo stdout un breve
rapporto sull'esito dei test (numero di operazioni effettuate, numero di operazioni concluse con successo,
numero di operazioni fallite, ecc.). Il formato di questo report è a discrezione dello studente.*/
#define _POSIX_C_SOURCE  200112L
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h> 
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include "error.h"
#include "requests.h"
#include "struct.h"
#include "commons.h"
#define MAXLEN 100000

char *strdup(const char *s);

int fd_socket;

/* test con sequenza di interi da 0 a 9 ripetuta size volte */
char *writeInt(size_t size) {
    int *a, j = 0;
    char *s;
    EC_NULL(a = calloc(size + 1, sizeof(int)), "calloc");
    EC_NULL(s = calloc(size + 1, sizeof(char)), "calloc");
    for (int i = 0; i < size; i++) {
        a[i] = i % 10;
        j += sprintf(&s[j], "%d", a[i]);
    }
    free(a);
    return s;
}

/* test con stringa prefissata */
char *writeChar(size_t size) {
    char *s, *p = "questa_e'_una_stringa_di_prova_";
    int len = strlen(p);
    EC_NULL(s = calloc(size + 1, sizeof(char)), "calloc");
    for (int i = 0; i < size; i++) {
        s[i] = p[i % len];
    }
    return s;
}

void func(char **argv) {
    char *username = *(argv + 1), *filename, *msg, *string, c;
    void *data;
    int operation = atoi(*(argv + 2)), sz_string;

    /* controllo che argv[1] sia un intero [1, 2, 3] */
    if (operation == 0) {
        ERROR("Test non valido");
    }
    
    /* salvo esito e nome delle op */
    outcomeOp *op = initialize();
   
    /* prima di eseguire le operazioni mi registro e mi connetto */

    /*  salvo il nome della operazione da passare alla struttura per il report finale.
        Per le operazioni di register e connect, poiché non fanno parte delle batterie di test,
        le inserisco solo nelle liste di successo o fallimento, senza incrementare il numero di successi o fallimenti */
    sz_string = 17 + strlen(username); 
    EC_NULL(string = (char *)calloc(sz_string, sizeof(char)), "calloc");
    snprintf(string, sz_string, "os_register(%s)", username);
    os_register(username) ? insert(&op -> succeededOpName, string, 0) : insert(&op -> failedOpName, string, 0);
    free(string);


    sz_string = 16 + strlen(username); 
    EC_NULL(string = (char *)calloc(sz_string, sizeof(char)), "calloc");
    snprintf(string, sz_string, "os_connect(%s)", username);
    os_connect(username) ? insert(&op -> succeededOpName, string, 0) : insert(&op -> failedOpName, string, 0);
    free(string);

    /* file in cui salverò i dati */
    EC_NULL(filename = (char *)calloc(2, sizeof(char)), "calloc");
    switch (operation) {        
        case 1:
            printf("Batteria n : 1\n");
            for (size_t size = 100, c = 'a'; c < 'u'; size += 4995, c++) {

                /* per l'ultima store aumento di più */
                if (c == 't') {
                    size = MAXLEN;
                }

                /* i nomi in cui salvo i dati sono le lettere dell'alfabeto */
                *filename = c;

                /* salvo op per report */
                sz_string = 14; 
                EC_NULL(string = (char *)calloc(sz_string, sizeof(char)), "calloc");
                snprintf(string, sz_string, "os_store(%s)", filename);

                /* se c è pari scrivo sequenza di numeri, altrimenti di stringhe */
                if (c % 2 == 0) {
                    msg = writeInt(size);
                    os_store(filename, msg, size) ? increaseSucceeded(op, string) : increaseFailed(op, string);
                } else {
                    msg = writeChar(size);
                    os_store(filename, msg, size) ? increaseSucceeded(op, string) : increaseFailed(op, string);
                }
                free(msg);
                free(string);
            }
            break;
        case 2:
            printf("Batteria n : 2\n");
            /* provo a leggere i file con nome a --> z e salvare il contenuto in data */
            for (c = 'a'; c <= 'z'; c++) {
                *filename = c;
                sz_string = 17; 
                EC_NULL(string = (char *)calloc(sz_string, sizeof(char)), "calloc"); 
                snprintf(string, sz_string, "os_retrieve(%s)", filename);
                data = os_retrieve(filename);
                if (data) {

                    /* controllo che il file, e il blocco restituito sia lo stesso */
                    char *filepath = mk_path("data", username, filename, NULL);

                    /* apro il file */
                    int fd;
                    EC_MENO1(fd = open(filepath, O_RDONLY), "opening");

                    /* estraggo la size del file */
                    struct stat file;
                    EC_MENO1(stat(filepath, &file), "stat");
                    size_t sz = file.st_size;

                    /* controllo la correttezza del retrieve */
                    void *block = (char*)calloc(sz + 1, sizeof(char));
                    read(fd, block, file.st_size);

                    (memcmp((char *)data, (char *)block, file.st_size) == 0) ? increaseSucceeded(op, string) : increaseFailed(op, string);
                    free(filepath);
                    free(block);
                    close(fd);

                /* file non presente */
                } else {
                    increaseFailed(op, string);
                }
                if (data) {
                    free(data);
                }
                free(string);
            }
            break;
        case 3:
            printf("Batteria n : 3\n");
            for (c = 'a'; c <= 'z'; c++) {
                /* provo a cancellare i file con nome a --> z */
                *filename = c;
                sz_string = 15; 
                EC_NULL(string = (char *)calloc(sz_string, sizeof(char)), "calloc"); 
                snprintf(string, sz_string, "os_delete(%s)", filename);
                (os_delete(filename)) ? increaseSucceeded(op, string) : increaseFailed(op, string);
                free(string);
            }      
            break;
        default: 
            ERROR("FAIL");
            break;
    }
    free(filename);
    printReportOp(op, username);
    os_disconnect();
}

void run_client(char **argv) {
    /* installo il signal handler per tutti i segnali che mi interessano */
    struct sigaction sa;

    /* resetto la struttura  */
    memset (&sa, 0, sizeof(sa));

    /* ignoro SIG_PIPE */
    sa.sa_handler = SIG_IGN;
    EC_MENO1(sigaction(SIGPIPE, &sa, NULL), "sigaction");

    struct sockaddr_un client;
    EC_MENO1(fd_socket = socket(AF_UNIX, SOCK_STREAM, 0), "creating server");
    memset(&client, '0', sizeof(client));
    client.sun_family = AF_UNIX;
    strncpy(client.sun_path, SOCKET_NAME, 108);

    /* se parte prima il client... */
    if (connect(fd_socket, (struct sockaddr *) &client, sizeof(client)) == -1 ) {
        ERROR("Server chiuso"); 
    }
    func(argv);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        exit(EXIT_FAILURE);   
    }
    run_client(argv);
    return EXIT_SUCCESS;   
}