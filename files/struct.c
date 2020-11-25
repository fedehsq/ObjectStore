#include <string.h>
#include "struct.h"

char *strdup(const char *s);

/* -------------------------- generiche funzioni di lista -------------------------- */

/* inserzione in testa */

void insert(node **t, char *name, int key) {
    node *p;
    EC_NULL(p = (node*)malloc(sizeof(node)), "malloc");
    p -> key = key;
    EC_NULL(p -> item = strdup(name), "strdup");
    if (*t) {
        p -> next = *t;
        *t = p;
    } else {
        *t = p;
        (*t) -> next = NULL;
    }
}

/* rimuove un elemento */
void destroy(node **t, int key) {
    if (*t) {
        if ((*t) -> key == key) {
            node *p = *t;
            *t = (*t) -> next;
            free(p -> item);
            free(p);
        } else {
            destroy(&(*t) -> next, key);
        }
    }
}

/* distrugge tutta la lista */
void destroyList(node *t) {
    if (t) {
        destroyList(t -> next);
        free(t -> item);
        free(t);
    }
}

/* controlla se user di chiave key è presente in t */
bool isIn(node *t, char *name) {
    if (!t) {
        return false;
    }
    return (strcmp(t -> item, name) == 0) ? true : isIn(t -> next, name); 
}

/* stampa elementi di lista */
void printList(node *t) {
    if (t) {
        printf("%s\n", t -> item);
        printList(t -> next);
    } else {
        puts("");
    }
}

/* -------------------------- gestione utenti connessi -------------------------- */
hashtable *mk_hasht(int size) {
    hashtable *ht;
    EC_NULL(ht = (hashtable*)malloc(sizeof(hashtable)), "malloc");
    ht -> nElem = 0;
    ht -> size = size;

    /* inizializzo "l'array verticale" e i mutex*/
    EC_IS_NOT_ZERO(pthread_mutex_init(&ht -> incr, NULL), "pthread_mutex_init");
    EC_NULL(ht -> list = (node **)malloc(size * sizeof(node *)), "malloc");
    EC_NULL(ht -> mutex =(pthread_mutex_t *)malloc(size * sizeof(pthread_mutex_t)), "malloc");
    for (int i = 0; i < size; i++) {
        ht -> list[i] = NULL;
        EC_IS_NOT_ZERO(pthread_mutex_init(&ht -> mutex[i], NULL), "pthread_mutex_init"); 
    }
    return ht;
}


/* crea hash */
unsigned hash(hashtable *ht, char *s) {
    unsigned hashval;
    for (hashval = 0; *s != '\0'; s++)
        hashval = *s + 31 * hashval;
    return hashval % ht -> size;
}

/* inserisce il nodo <name, key> */
void insertUser(hashtable *ht, char *name, int key) {
    int i = hash(ht, name);
    /* prendo il lock sulla i-esima lista */
    pthread_mutex_lock(&ht -> mutex[i]);
    insert(&ht -> list[i], name, key);
    pthread_mutex_unlock(&ht -> mutex[i]);

    /* anche questa variabile è condivisa, mi serve il lock */
    pthread_mutex_lock(&ht -> incr);
    ht -> nElem++;
    pthread_mutex_unlock(&ht -> incr);
}

/* rimuove il nodo associato alla chiave key */
void removeUser(hashtable *ht, char *name, int key) {
    int i = hash(ht, name);
    pthread_mutex_lock(&ht -> mutex[i]);
    destroy(&ht -> list[i], key);
    pthread_mutex_unlock(&ht -> mutex[i]);

    /* anche questa variabile è condivisa, mi serve il lock */
    pthread_mutex_lock(&ht -> incr);
    ht -> nElem--;
    pthread_mutex_unlock(&ht -> incr);
}

bool isConnected(hashtable *ht, char *name, int key) {
    int i = hash(ht, name);
    return isIn(ht -> list[i], name);
}

void printUserList(hashtable *ht) {
    for (int i = 0; i < ht -> size; i++) {
        pthread_mutex_lock(&ht -> mutex[i]);
    }
    for (int i = 0; i < ht -> size; i++) {
        if (ht -> list[i]) {
            printf("\n\n\n%d: ", i);
            printList(ht -> list[i]);
        }
    }
    for (int i = 0; i < ht -> size; i++) {
        pthread_mutex_unlock(&ht -> mutex[i]);
    }
}

/* -------------------------- gestione esito operazioni -------------------------- */


/* utilizzo una semplice lista linkata per salvare gli esiti */
outcomeOp *initialize() {
    outcomeOp *operations;
    EC_NULL(operations = (outcomeOp *)malloc(sizeof(outcomeOp)), "malloc");
    operations -> nOperations = 0;
    operations -> failedOp = 0;
    operations -> succeededOp = 0;
    operations -> succeededOpName = operations -> failedOpName = NULL;
    return operations;
}

void increaseSucceeded(outcomeOp *operations, char *name) {
    operations -> nOperations++;
    operations -> succeededOp++;
    insert(&operations -> succeededOpName, name, 0);
}

void increaseFailed(outcomeOp *operations, char *name) {
    operations -> nOperations++;
    operations -> failedOp++;
    insert(&operations -> failedOpName, name, 0);
}

void printReportOp(outcomeOp *operations, char *username) {
    printf("User : %s\n", username);
    printf("Operazioni effettuate per batteria : %d\n", operations -> nOperations);
    printf("Operazioni effettuate con successo per batteria : %d\n", operations -> succeededOp);
    printf("Operazioni fallite per batteria : %d\n", operations -> failedOp);
    printf("Le seguenti operazioni sono state eseguite con successo:\n");
    printList(operations -> succeededOpName);
    destroyList(operations -> succeededOpName);
    printf("Le seguenti operazioni hanno fallito:\n");
    printList(operations -> failedOpName);
    puts("");
    destroyList(operations -> failedOpName);
    free(operations);
}
