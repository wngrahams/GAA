/*
 * crossover.h
 * Contains multiple different crossover methods to be used in GAA.c
 *
 */

#ifndef _CROSSOVER_H_
#define _CROSSOVER_H_

#include "bitarray.h"
#include "ga-params.h"

/*
 * Single Point Crossover: With probabilty SP_CROSSOVER_PROB, cross over the
 * pair of individuals at a randomly chosen point (TODO: chosen with uniform
 * probability) to form two offspring. If no crossover takes place, form two
 * offspring  that are exact copies of their respective parents.
 */
static inline void single_point_crossover(Individual* pop,
                                          int parent_idxs[],
										  int num_nodes,
                                          Individual* child1,
                                          Individual* child2) {

    double crossover_decision = (double)rand()/RAND_MAX;
    if (crossover_decision < CROSSOVER_PROB) {

        //printf("Performing crossover... ");

        // choose the bit at which to crossover: (TODO better random)
        // doesn't pick bit 0 because that is the same as no crossover
        int crossover_point = (rand()%(num_nodes-1)) + 1;  // (0, num_nodes)

        /*
        printf("Crossover point = %d\n", crossover_point);
        printf("\tParents: ");
        for( int j=0; j<num_nodes; j++) {
            printf("%d", getbit(pop[parent_idxs[0]].partition, j));
        }
        printf(", ");
        for( int j=0; j<num_nodes; j++) {
            printf("%d", getbit(pop[parent_idxs[1]].partition, j));
        }
        printf("\n");
        */

        // fill in the non-crossover part of the bitarray as a copy of 
        // the parents:
        for (int bit_to_copy=0; bit_to_copy<crossover_point; bit_to_copy++) {
                    
            putbit(child1->partition,
                   bit_to_copy,
                   getbit(pop[parent_idxs[0]].partition, bit_to_copy)
                  );
            putbit(child2->partition,
                   bit_to_copy,
                   getbit(pop[parent_idxs[1]].partition, bit_to_copy)
                  );
        }

        // do the crossover:
        for (int bit_to_swap=crossover_point; 
			 bit_to_swap<num_nodes; 
             bit_to_swap++) {

             putbit(child1->partition, 
                    bit_to_swap, 
                    getbit(pop[parent_idxs[1]].partition, bit_to_swap)
                   );
            putbit(child2->partition,
                   bit_to_swap,
                   getbit(pop[parent_idxs[0]].partition, bit_to_swap)
                  );
        }

        /*
        printf("\tChildren: ");
        for (int j=0; j<num_nodes; j++) {
            printf("%d", getbit(child1->partition, j));
        }
        printf(", ");
        for (int j=0; j<num_nodes; j++) {
            printf("%d", getbit(child2->partition, j));
        }
        printf("\n");
        */

    }
    else {

        //printf("No crossover.. children will be copies of parents.\n");
        // the two children are exact copies of the parents
        for (int j=0; j<RESERVE_BITS(num_nodes); j++) {
             child1->partition[j] = pop[parent_idxs[0]].partition[j];
             child2->partition[j] = pop[parent_idxs[1]].partition[j];
        }       

    }
}

#endif /* _CROSSOVER_H_ */ 

