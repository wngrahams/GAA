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


typedef struct Individual {
    bitarray_t* partition;  // array of bits representing partition
    int fitness;                // fitness of individual's solution
} Individual;


int calc_fitness(Graph*, Individual*);
void shuffle(int *, int);

#endif  /* _GAA_H */

