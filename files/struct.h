#include <pthread.h>
#include "error.h"

/* linked list */
typedef struct _node {
    int key;
    char *item;
    struct _node *next;
} node;

/* struttura in cui salvo gli utenti connessi in sessione */
typedef struct hashtable_ {
    node **list;
    int nElem;
    int size;
    /* avrò un mutex per ogni lista della tabella hash */
    pthread_mutex_t *mutex;
    
    /* mutex per aumentare la variabile nElem */
    pthread_mutex_t incr;
} hashtable;

/* struttura in cui salvo l'esito delle operazioni */
typedef struct outcomeOp_ {
    /* numero op  */
    int nOperations;

    /*numero op fallite */
    int failedOp;

    /* numero op a buon fine */
    int succeededOp;

    /* lista che salva le op fallite */
    node *failedOpName;

    /* lista che salva le op a buon fine */
    node *succeededOpName;
} outcomeOp;

/* ----------- operazioni generiche liste ------------ */

/* inserisce in testa l'elemento di chiave key */
void insert(node **, char *, int);

/* rimuove l'elemento dalla lista di chiave key */
void destroy(node **, int);

/* distrugge tutta la lista */
void destroyList(node *);

/* stampa elementi di lista */
void printList(node *);

/* controlla se l'elemento item è presente */
bool isIn(node *, char *);

/* ----------- gestione operazioni utenti lato client ------------ */

/* inizializzo */
outcomeOp *initialize();

/*  se l'operazione ha avuto successo incremento succeededOp, 
    e salvo il nome dell'operazione in lista */
void increaseSucceeded(outcomeOp *, char *);

/*  se l'operazione ha avuto successo incremento failedOp, 
    e salvo il nome dell'operazione in lista */
void increaseFailed(outcomeOp *, char *);

/* stampo su stdout e su file il report */
void printReportOp(outcomeOp *, char *);


/* ----------- gestione utenti lato server ------------ */

/* inizializza il buffer contente gli user connessi */
hashtable *mk_hasht();

// da fare in mutua esclusione
/* quando un user si connette lo aggiungo alla coda utenti connessi */
void insertUser(hashtable *, char *, int);

/*  quando un user si disconnette, 
    lo rimuovo dalla coda utenti connessi tramite la chiave a lui associata */
void removeUser(hashtable *, char *, int); 

/* controlla se un utente è già connesso */
bool isConnected(hashtable *, char *, int);

/* stampa gli utenti online */
void printUserList(hashtable *);

/* crea hash */
unsigned hash(hashtable *, char *);