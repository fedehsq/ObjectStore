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
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdarg.h>
#include <signal.h>
#include "struct.h"
#include "commons.h"

/* lock per controllare la directory data, alla quale devo accedere in mutua esclusione*/
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* la setto a true quando ricevo un segnale */
volatile bool KILL_SERVER = false;

/* la setto a true quando ricevo SIGUSR1 */
volatile bool _SIGUSR1_ = false;

/*  salva gli utenti connessi in sessione */
hashtable *ht;

char *strdup(const char *s);

/*  stampa: nome utente registrato, numero e nome dei suoi files,
    returna il numero dei files */
void printUserInfo(char *username, int *objects, size_t *sizeStore);

/*  stampa: numero di client attualemente connessi, numero di client registrati,
    numero di oggetti nello store e size totale dello store */
void printServerInfo();

/*  controlla se  "fl" è presente in "folder" 
    fl può essere un file, mentre folder una directory */
bool isInDir(char *fl, char *folder);

/*  creo la cartella dell'utente desiderato se non è già presente
    (lo registro allo store) */
bool subscribe(char *user);

/* tokenizza il nome del file, il blocco che va salvato nel file e la sua size */
char* token(char *s, char **arg, size_t *len);

/*  crea il path desiderato */
char *mk_path(char *name, ...);

/* gestisce le richieste dei client */
void *startFunc(void *arg);

/* crea il thread che starta il client */
void startClient(long fdc);


/* stampa nome utente registrato, numero e nome dei suoi files */
void printUserInfo(char *username, int *objects, size_t *sizeStore) {
    int nelem = 0;
    size_t sizeUserStore = 0;
    DIR *user;
    struct dirent *item;
    struct stat info;
    EC_MENO1(chdir(username), "chdir");
    EC_NULL(user = opendir("."), "opening");

    printf("Utente %s con dati:\n", username);
    while ((errno = 0, item = readdir(user))) {
        /* ignoro le dir "." e ".." */
        if (strcmp(item -> d_name, ".") == 0 || strcmp(item -> d_name, "..") == 0) {
            continue;
        }
        EC_MENO1(stat(item -> d_name, &info), "stat");
        sizeUserStore += info.st_size;
        ++nelem;
        printf("%s ", item -> d_name);
    }
    printf("\nSize dei file di %s: %ld kb, ha: %d files\n", username, sizeUserStore, nelem);
    EC_MENO1(closedir(user), "closing dir");

    /* torno in data */
    EC_MENO1(chdir(".."), "chdir"); 

    /* sommo ai totali, quelle dell'attuale utente */
    *objects += nelem;
    *sizeStore += sizeUserStore;
    puts("");
}


/*  numero di client attualemente connessi, numero di client registrati
    numero di oggetti nello store e size totale dello store */
void printServerInfo() {
    size_t sizeStore = 0;
    int registeredUser = 0, objects = 0;

    /* stampa gli utenti attualmente connessi */
    printUserList(ht);

    DIR *data;
    struct dirent *item;
    struct stat info;

    /* posso spostarmi tra le dir perché non ho più thread dispatcher */
    EC_MENO1(chdir("./data"), "chdir");
    EC_NULL(data = opendir("."), "opening");

    /* itero in "data" finché non ho esaminato tutti gli utenti */
    while ((errno = 0, item = readdir(data))) {
        EC_MENO1(stat(item -> d_name, &info), "stat");

        /* mi assicuro che sia una directory */
        if (S_ISDIR(info.st_mode)) {

            /* ignoro le dir "." e ".." */
            if (strcmp(item -> d_name, ".") == 0 || strcmp(item -> d_name, "..") == 0) {
                continue;
            }
            /* itera sull'utente item -> d_name */ 
            printUserInfo(item -> d_name, &objects, &sizeStore);

            ++registeredUser;
        }
    }
    printf("Utenti attualmente connessi: %d\n", ht -> nElem);
    printf("Client registrati: %d\nOggetti totali: %d\nSize totale dello store: %ld KB\n", registeredUser, objects, sizeStore);

    EC_MENO1(closedir(data), "closing dir");

    /* torno nella working directory dello store */
    EC_MENO1(chdir(".."), "chdir"); 
}

/*  controlla se  "fl" è presente in "folder" 
    fl può essere qualsiasi cosa mentre folder una dir */
bool isInDir(char *fl, char *folder) {

    /*  inizialmente ho approcciato cambiando wd, ma avevo problemi con più client connessi 
        ho optato per salvare il path completo per iterare */
    int size, found = false;
    DIR *dir;
    struct dirent *file; 
    char wd[256], *path;
    
    /* salvo la wd per concatenarci la cartella nella quale iterare */
    getcwd(wd, 256);

    size = strlen(wd) + strlen(folder) + 6;
    EC_NULL(path = (char *)calloc(size, sizeof(char)), "calloc");

    /* copio il path completo in "path" */
    snprintf(path, size, "%s/%s", wd, folder);

    while ((dir = opendir(path)) == NULL) {
        /* troppi fd aperti, attendo la chiusura */
        if (errno == EMFILE) {
            continue;
        } else {
            EC_NULL(dir, "opendir");
        }
    }
    while ((errno = 0, file = readdir(dir))) {
        if (strcmp(file -> d_name, fl) == 0) {
            found = true;
            break;
        } 
    }
    closedir(dir);
    free(path);
    return found;     
} 

/*  creo la cartella dell'utente desiderato se non è già presente
    (lo registro allo store), controllo in modo atomico */
bool subscribe(char *user) {
    bool found = false;
    pthread_mutex_lock(&mutex);
    if (!isInDir(user, "data")) {
        char *path = mk_path("data", user, NULL);
        if (mkdir(path, 0777) == -1) {
            perror("mkdir");
            free(path);
            found = false;
        }
        free(path);
        found = true;
    }
    pthread_mutex_unlock(&mutex);
    return found;
}

/* gestisce le richieste dei client */
void *startFunc(void *arg) {
    long fdc = (long)arg;
    char *user = NULL; 
    bool connected = false, leave = false;
    for (;;) {

        char *buf = NULL, *filepath = NULL, *userpath = NULL, *filename = NULL;
        char **splitted_request = NULL;
        
        /* leggo richiesta dal client */
        buf = readFrom(fdc);
        
        /* spezzo il messaggio ricevuto */
        splitted_request = tokenizer(buf);
        /* verifico che la richiesta sia valida */
        if (splitted_request == NULL) {
            /* se fallisco la scrittura il client si è interrotto in maniera anomala */
            if (writeTo("Operazione non consentita", fdc) == -1) {
                goto disconnect;
            }
        }

        if (strncmp(splitted_request[0], "REGISTER", 8) == 0) {
            
            /* controllo se l'utente è gia registrato */
            if (subscribe(splitted_request[1])) {
                if (writeTo("OK", fdc) == -1) {
                    goto disconnect;
                }
            }
            else {
                if (writeTo("KO utente gia' registrato", fdc) == -1) {
                    goto disconnect;
                }
            }

        } else if (strncmp(splitted_request[0], "CONNECT", 7) == 0) {
                            
            /* controllo che l'utente sia registrato */
            if (isInDir(splitted_request[1], "data")) {

                /*  controllo che non sia già connesso, ovvero che non si trovi già nella tabella */
                if (!isConnected(ht, splitted_request[1], fdc)) {

                    /* copio user perché mi servirà per le prossime op */
                    if ((user = strdup(splitted_request[1])) == NULL) {
                        perror("strdup");
                        goto disconnect;
                    }
                    
                    /* passo alla ht il nome e la chiave da inserire */
                    insertUser(ht, user, fdc);

                    printf("%s si e' connesso\nUtenti attualmente connessi: %d\n", user, ht -> nElem);
                    fflush(stdout);

                    /* adesso l'utente è connesso e può eseguire le op */
                    connected = true;
                    if (writeTo("OK", fdc) == -1) {
                        goto disconnect;
                    }
                } else {
                    /* si trova già nel buffer */
                    if (writeTo("Utente gia' connesso", fdc) == -1) {
                        goto disconnect;
                    } 
                }
            } else {
                /* isInDir() ha returnato false (utente non registrato) */ 
                if (writeTo("Utente non registrato", fdc) == -1) {
                    goto disconnect;
                }
            }
        } else {

            /* per queste devo essere necessariamente registrato e connesso */
            if (connected) {
                /* file descriptor del file, necessario per os_store() e os_retrieve */
                int fd;
                
                /* blocco di dati */
                void *data = NULL;
                
                /* dimensione del file data */
                size_t len = 0;
                /* lo uso per controllare la conversione da char* a int */
                char *endptr;

                if (strncmp(splitted_request[0], "STORE", 5) == 0) {

                    /*  tokenizza il filename, la size e data */
                    filename = splitted_request[1];
                    len = strtol(splitted_request[2], &endptr, 10);

                    /* la conversione ha fallito, non ho passato un numero */
                    if (*endptr != '\0') {
                        if (writeTo("size invalida, riprova", fd) == -1) {
                            goto disconnect;
                        }
                        continue;
                    }

                    data = splitted_request[3];

                    /* creo il path  ".../wd/data/user" */
                    userpath = mk_path("data", user, NULL);

                    /* controllo che non esista già il file in "./data/user" */
                    if (!isInDir(filename, userpath)) { 

                        /*  per creare il file non mi muovo dalla wd: concateno i path 
                            creo il path  ".../wd/data/user" */
                        filepath = mk_path("data", user, filename, NULL);

                        /* creo il file, lo apro, ci scrivo il blocco */
                        while ((fd = open(filepath, O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU)) == -1) {
                            /* troppi fd aperti, attendo la chiusura */
                            if (errno == EMFILE) {
                                continue;
                            } else {
                                perror("open");
                                goto disconnect;
                            }
                        }
                        if (writen(fd, data, len) == -1) {
                            goto disconnect;
                        }

                        /* lo chiudo, lo riapro e controllo corretto salvataggio */
                        close(fd);

                        /* se ho troppi fd aperti, attendo */
                        while ((fd = open(filepath, O_RDONLY)) == -1) {
                            if (errno == EMFILE) {
                                continue;
                            } else {
                                perror("open");
                                goto disconnect;
                            }
                        }

                        /* controllo che il blocco passato sia stato salvato correttamente */
                        struct stat file;
                        if (stat(filepath, &file) == -1) {
                            perror("stat");
                            goto disconnect;
                        }
                        size_t sz = file.st_size;
                        void *buff = (char*)calloc(sz + 1, sizeof(char));
                        if (readn(fd, (char *)buff, sz) == -1) {
                            goto disconnect;
                        }
                        close(fd);
                        if (memcmp((char *)data, (char *)buff, sz) == 0) {
                            if (writeTo("OK", fdc) == -1) {
                                goto disconnect;
                            }
                        } else {
                            /* lo rimuovo perché il blocco salvato non è quello desiderato */
                            unlink(filepath);
                            if (writeTo("KO: file corrotto", fdc) == -1) {
                                goto disconnect;
                            }
                        }
                        free(buff);
                    } else {
                        /* inInDir() ha returnato true (file già presente) */
                        if (writeTo("File gia' presente", fdc) == -1) {
                            goto disconnect;
                        }
                    }
                } else if (strncmp(splitted_request[0], "RETRIEVE", 8) == 0) {
                    void *buff;

                    /*  estraggo la size dello store,
                        necessaria per allocare la giusta dimensione per il blocco */
                    size_t sz;
                    struct stat file;

                    filename = splitted_request[1];

                    /* creo il path ".../wd/data/user" */
                    userpath = mk_path("data", user, NULL);

                    /* controllo che il file richiesto ci sia */
                    if (isInDir(filename, userpath)) {

                        /* creo il path ".../wd/data/user/filename" */
                        filepath = mk_path("data", user, filename, NULL);

                        /* apro il file "filepath" */
                        while ((fd = open(filepath, O_RDONLY)) == -1) {
                            /* troppi fd aperti, attendo la chiusura */
                            if (errno == EMFILE) {
                                continue;
                            } else {
                                perror("open");
                                goto disconnect;
                            }
                        }

                        /* estraggo la size del file, con la quale alloco data */
                        if (stat(filepath, &file) == -1) {
                            perror("stat");
                            goto disconnect;
                        }
                        len = file.st_size + 1;
                        if ((data = (char *)calloc(len, sizeof(char))) == NULL) {
                            perror("calloc");
                            close(fd);
                            goto disconnect;
                        }

                        /* scrivo in data il contenuto del file */
                        if ((readn(fd, data, len) == -1)) {
                            close(fd);
                            goto disconnect;
                        }
                        close(fd);

                        /* dimensione da passare al client per una corretta allocazione 
                           "DATA %ld \n%s", alloco il buffer e lo passo al client */
                        sz = 14 + len;
                        if ((buff = (char*)calloc(sz, sizeof(char))) == NULL) {
                            perror("calloc");
                            goto disconnect;
                        }
                        
                        snprintf((char*) buff, sz, "DATA %ld \n%s", len, (char *)data);
                        if (writeTo(buff, fdc) == -1) {
                            free(data);
                            free(buff);  
                            goto disconnect;
                        }
                        free(data);
                        free(buff);
                    } else {
                        /* inInDir() ha returnato false (file non presente) */
                        if (writeTo("File non presente", fdc) == -1) {
                            goto disconnect;
                        }
                    }
                } else if (strncmp(splitted_request[0], "DELETE", 6) == 0) {
                    int removed;
                    filename = splitted_request[1];

                    /* creo il path "./data/user/filename" */
                    filepath = mk_path("data", user, filename, NULL);
                    
                    /*  invece che usare "isInDir" per controllare se il file è presente,
                        provo la rimozione e controllo testando errno  */
                    removed = unlink(filepath);

                    /* file non esistente o altro errore */
                    if (removed == -1) {
                        if (errno == ENOENT) {
                            if (writeTo("KO file non esistente", fdc) == -1) {
                                goto disconnect;
                            }
                        } else { 
                            if (writeTo("KO errore col file", fdc) == -1) {
                                goto disconnect;
                            }
                        }
                    } else {
                        /* rimozione effetuata */
                        if (writeTo("OK", fdc) == -1) {
                            goto disconnect;
                        }
                    }
                } else if (strncmp(splitted_request[0], "LEAVE", 5) == 0) {
                    /* richiesta esplicita del client oppure arrivato un segnale */
                    disconnect:
                    connected = false;
                    leave = true;
                    /* rimuovo l'utente dalla ht degli utenti attualmente connessi */  
                    if (user) {
                        removeUser(ht, user, fdc);
                        printf("%s si e' disconnesso\nUtenti attualmente connessi: %d\n", user, ht -> nElem);
                        fflush(stdout);
                        free(user);
                    }
                    if (writeTo("OK", fdc) != -1) {
                        close(fdc);     
                    }
                }
            }
            else {
                /* connected = false */
                if (writeTo("Utente non connesso", fdc) == -1) {
                    goto disconnect;
                }
            }
        }

        if (buf) {
            free(buf);
            buf = NULL;
        }
        if (filepath) {
            free(filepath);
            filepath = NULL;
        }
        if (userpath) {
            free(userpath);
            userpath = NULL;
        }
        if (splitted_request) {
            destroyBuff(splitted_request);
            free(splitted_request);
            splitted_request = NULL;
        }
        if (leave) {
            break;
        }
    }
    return NULL;
}


/* crea il thread che starta il client */
void startClient(long fdc) {
    pthread_t id;
    pthread_attr_t attr;

    /* maschero i segnali per i client, il main gestirà i segnali */
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT); 
    sigaddset(&mask, SIGPIPE);
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGTSTP);
    sigaddset(&mask, SIGUSR1);

    if (pthread_sigmask(SIG_BLOCK, &mask,&oldmask) != 0) {
        fprintf(stderr, "FATAL ERROR\n");
        close(fdc);
        return;
    }

    if (pthread_attr_init(&attr) != 0) {
        fprintf(stderr, "pthread_attr_init FALLITA\n");
        close(fdc);
        return;
    }

    // settiamo il thread in modalità detached
    if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0) {
        fprintf(stderr, "pthread_attr_setdetachstate FALLITA\n");
        pthread_attr_destroy(&attr);
        close(fdc);
	    return;
    }
    if (pthread_create(&id, &attr, startFunc, (void*)fdc) != 0) {
        fprintf(stderr, "pthread_create FALLITA\n");
        pthread_attr_destroy(&attr);
        close(fdc);
    }
    
    if (pthread_sigmask(SIG_SETMASK, &oldmask, NULL) != 0) {
	    fprintf(stderr, "FATAL ERROR\n");
	    close(fdc);	
    }
}

static void gHandler() {
    write(1, "Ricevuto un segnale, termino i client lasciando il sistema in uno stato consistente\n", 85);
    KILL_SERVER = true;
}


static void sHandler(int signal) {
    write(1, "Ricevuto SIGUSR1, stampo info sullo STORE e termino\n", 53);
    KILL_SERVER = true;
    _SIGUSR1_ = true;
}

int main(int argc, char **argv) {

    /* cancello sockname */
    unlink(SOCKET_NAME);
     
    /* installo il signal handler per tutti i segnali che mi interessano */
    struct sigaction sa;

    /* resetto la struttura  */
    memset (&sa, 0, sizeof(sa));

    /* ignoro SIG_PIPE */
    sa.sa_handler = SIG_IGN;
    EC_MENO1(sigaction(SIGPIPE, &sa, NULL), "sigaction");

    /* per SIGUSR1 setto la variabile che mi farà stampare le info sullo store, poi termina */
    sa.sa_handler = sHandler;
    EC_MENO1(sigaction(SIGUSR1, &sa, NULL), "sigaction");

    /*  per questi segnali faccio terminare in uno stato consistente
        usando lo stesso handler */
    sa.sa_handler = gHandler;
    EC_MENO1(sigaction(SIGINT, &sa, NULL), "sigaction");
    EC_MENO1(sigaction(SIGTSTP, &sa, NULL), "sigaction");
    EC_MENO1(sigaction(SIGQUIT, &sa, NULL), "sigaction");

    /* creo la dir "data" se non esiste già */
    if (mkdir("./data", 0777) == -1 && errno != EEXIST) {
        ERROR("errore nella creazione");
    }

    /* inizializzo la tabella hash contenente gli user connessi */
    ht = mk_hasht(100);

    int fd_socket;
    struct sockaddr_un server;
    
    /* crea server  */
    EC_MENO1(fd_socket = socket(AF_UNIX, SOCK_STREAM, 0), "creating server");
    memset(&server, '0', sizeof(server));
    puts("[+] server creato");

    server.sun_family = AF_UNIX;
    strncpy(server.sun_path, SOCKET_NAME, 108);

    /* assegno nome al socket */
    EC_MENO1(bind(fd_socket, (struct sockaddr *)&server, sizeof(server)), "binding"); 
    puts("[+] assegno nome al server");

    /* in ascolto */
    EC_MENO1(listen(fd_socket, 8), "listening");
    puts("[+] in ascolto...");

    for (;;) {
        long fd;

        /*  quando ricevo un segnale smetto di accettare connessioni in arrivo
            e attendo che i client connessi terminino le loro operazioni */
        if (KILL_SERVER == true) {

            /* non ho più client connessi */
            if (ht -> nElem == 0) {

                /* il segnale ricevuto è SIGUSR1 */
                if (_SIGUSR1_ == true) {
                    printServerInfo();
                }
                puts("non ci sono piu' client connessi, posso terminare");
                break;
            }
        } else {
            if ((fd = accept(fd_socket, (struct sockaddr *)NULL ,NULL)) == -1) {
                if (errno == EINTR) {
                    /* controllare SIGUSR1 */
                    continue;
                }
            }
            startClient(fd);
        }
    }
    /* libero la ht degli utenti */
    free(ht -> list);
    free(ht -> mutex);
    free(ht);

    /* chiudo il server */
    close(fd_socket);
    puts("[+] Server chiuso\n");

    /* per distruzione thread */
    sleep(1);
    return(EXIT_SUCCESS);
}