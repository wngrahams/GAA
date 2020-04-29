/*
 * ga-params.h
 *
 * Header file for all Genetic Algoritm Parameters
 *
 */

#ifndef _GA_PARAMS_H_
#define _GA_PARAMS_H_

#include "bitarray.h"

#define CROSSOVER_PROB 0.85
#define MUTATION_PROB 0.001
#define PUC_PROB 0.7
#define TOURNAMENT_SELECT_PROB 0.75

#define NUM_OF_GENERATIONS 160
#define POP_SIZE 60

#define NUM_ISLANDS 3
#define MIGRATION_PERIOD 50
#define PROB_ISLAND_STAY 0.75
#define PROB_ISLAND_MIGRATE (double)(1 - PROB_ISLAND_STAY)/(NUM_ISLANDS - 1)
#define PROB_ISLAND_REWARD 0.05
#define PROB_ISLAND_PENALTY 0.05

typedef struct Individual {
    bitarray_t* partition;  // array of bits representing partition
    int fitness;            // fitness of individual's solution
} Individual;

#endif /* _GA_PARAMS_H_ */

