/*
 * GAA.h
 *
 * Header file for GAA.c
 *
 */

#ifndef _GAA_H_
#define _GAA_H_

#include "bitarray.h"
#include "graph.h"

#define CHECK_MALLOC_ERR(ptr) ((!check_malloc_err(ptr)) ? (exit(1)) : (1))

// file type macros
#define EL    0
#define WEL   1
#define DOT   2
#define GZ    3
#define GRAPH 4

typedef struct Individual {
    bitarray_t* partition;  // array of bits representing partition
    int fitness;                // fitness of individual's solution
} Individual;

/*
 * This function checks if malloc() returned NULL. If it did, the program
 * prints an error message. The function returns 1 on success and 0 on failure
 */
static inline int8_t check_malloc_err(const void *ptr) {
    if (NULL == ptr) {
        perror("malloc() returned NULL");
        return 0;
    } /* END if */

    return 1;
}

int calc_fitness(Graph*, Individual*);
void shuffle(int *, int);

#endif  /* _GAA_H */

