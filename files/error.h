/* AN HEADER FILE THAT CONTAINS SOME ERRORS MACRO */
#include <stdlib.h>
#include <stdio.h>
#define bool int
#define true 1
#define false 0
// a malloc for int array + CHECK operation
#define INT_MALLOC(A, SIZE) EC_NULL((A) = (int *)malloc((SIZE) * sizeof(int)), "malloc")
// a malloc for char array + CHECK operation
#define CHAR_MALLOC(A, SIZE) EC_NULL((A) = (char *)malloc((SIZE) * sizeof(char)), "malloc")
// a realloc for char array + CHECK operation
#define CHAR_REALLOC(A, SIZE) EC_NULL((A) = (char *)REALLOC((A), SIZE), "realloc")
// a simple print of passed string
#define ERROR(S) { fprintf(stderr, "%s\n", S); exit(-1); }
/* controlla -1; stampa errore e termina */
#define EC_MENO1(S, M) if ((S) == -1) {fprintf(stderr, "Line %d ", __LINE__);  perror(M); exit(EXIT_FAILURE);}
/* controlla NULL; stampa errore e termina */
#define EC_NULL(S, M) if ((S) == NULL) {fprintf(stderr, "Line %d ", __LINE__); perror(M);  exit(EXIT_FAILURE);}
/* controlla 0; stampa errore e termina */
#define EC_ZERO(S, M) if ((S) == 0) {fprintf(stderr, "Line %d ", __LINE__); perror(M);  exit(EXIT_FAILURE);}
/* controlla !0; stampa errore e termina */
#define EC_IS_NOT_ZERO(S, M) if ((S) != 0) {fprintf(stderr, "Line %d ", __LINE__); perror(M);  exit(EXIT_FAILURE);}
/* stampa linea */
#define PRINT_LINE fprintf(stdout, "This is the line: %d\n", __LINE__);
