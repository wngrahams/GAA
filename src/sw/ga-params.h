/*
 * ga-params.h
 *
 * Header file for all Genetic Algoritm Parameters
 *
 */

#ifndef _GA_PARAMS_H_
#define _GA_PARAMS_H_

#include "bitarray.h"

#define CROSSOVER_PROB 0.7
#define MUTATION_PROB 0.001
#define TOURNAMENT_SELECT_PROB 0.75

#define NUM_OF_GENERATIONS 10000
#define POP_SIZE 100

typedef struct Individual {
    bitarray_t* partition;  // array of bits representing partition
    int fitness;            // fitness of individual's solution
} Individual;

#endif /* _GA_PARAMS_H_ */

