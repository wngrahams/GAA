/*
 * GAA.h
 *
 * Header file for GAA.c
 *
 */

#ifndef _GAA_H_
#define _GAA_H_

#include "bitarray.h"

#define CHECK_MALLOC_ERR(ptr) ((!check_malloc_err(ptr)) ? (exit(1)) : (1))

// file type macros
#define EL    0
#define WEL   1
#define DOT   2
#define GZ    3
#define GRAPH 4

// genetic macros
#define NUM_OF_GENERATIONS 1

typedef struct Node {
    int id;      // node id
    int weight;  // node weight
} Node;

typedef struct Edge {
    int n1;      // id of node 1
    int n2;      // id of node 2
    int weight;  // edge weight
} Edge;

typedef struct Graph {
    int v;         // number of nodes
    int e;         // number of edges
    Node** nodes;  // array of pointers to nodes
    Edge** edges;  // array of pointers to edges
} Graph;

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

