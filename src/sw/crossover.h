/*
 * crossover.h
 * Contains multiple different crossover methods to be used in GAA.c
 *
 */

#ifndef _CROSSOVER_H_
#define _CROSSOVER_H_

#include "bitarray.h"
#include "ga-params.h"
#include "ga-utils.h"


/* Returns a crossover position chosen from a uniform random distribution 
 * between 0 and num_nodes (exlusive)
 */
static inline int _get_crossover_position(int num_nodes) {
    //return (rand()%(num_nodes-1)) + 1;
    return urandint(num_nodes-1) + 1;
}


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

        // choose the bit at which to crossover:
        // doesn't pick bit 0 because that is the same as no crossover
        int crossover_point = _get_crossover_position(num_nodes);  
        // chooses a point between (0, num_nodes) exclusive

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
} /* END Single Point Crossover */


/*
 * Two Point Crossover: With probabilty SP_CROSSOVER_PROB, cross over the
 * pair of individuals at two randomly chosen point (TODO: chosen with uniform
 * probability) to form two offspring. If no crossover takes place, form two
 * offspring  that are exact copies of their respective parents.
 */
static inline void two_point_crossover(Individual* pop,
                                       int parent_idxs[],
                                       int num_nodes,
                                       Individual* child1,
                                       Individual* child2) {

    double crossover_decision = (double)rand()/RAND_MAX;
    if (crossover_decision < CROSSOVER_PROB) {

        int pos1 = _get_crossover_position(num_nodes);
        int pos2 = _get_crossover_position(num_nodes);

        // fill in the non-crossover part of the bitarray as a copy of 
        // the parents:
        for (int bit_to_copy=0; bit_to_copy<MIN(pos1,pos2); bit_to_copy++) {
                    
            putbit(child1->partition,
                   bit_to_copy,
                   getbit(pop[parent_idxs[0]].partition, bit_to_copy)
                  );
            putbit(child2->partition,
                   bit_to_copy,
                   getbit(pop[parent_idxs[1]].partition, bit_to_copy)
                  );
        }

        // do the crossover between the two points:
        for (int bit_to_swap=MIN(pos1,pos2); 
			 bit_to_swap<MAX(pos1,pos2); 
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

        // fill in the rest of the bitarray as a copy of the parents
        for (int bit_to_copy=MAX(pos1,pos2); 
             bit_to_copy<num_nodes; 
             bit_to_copy++) {
            
            putbit(child1->partition,
                   bit_to_copy,
                   getbit(pop[parent_idxs[0]].partition, bit_to_copy)
                  );
            putbit(child2->partition,
                   bit_to_copy,
                   getbit(pop[parent_idxs[1]].partition, bit_to_copy)
                  );
        }
    }
    else {

        //printf("No crossover.. children will be copies of parents.\n");
        // the two children are exact copies of the parents
        for (int j=0; j<RESERVE_BITS(num_nodes); j++) {
             child1->partition[j] = pop[parent_idxs[0]].partition[j];
             child2->partition[j] = pop[parent_idxs[1]].partition[j];
        }       

    }
} /* END Two Point Crossover */


/*
 * Paramaterized Uniform Crossover: An exchange happens at each bit position
 * with probability PUC_PROB
 */
static inline void parameterized_uniform_crossover(Individual* pop,
                                                   int parent_idxs[],
                                                   int num_nodes,
                                                   Individual* child1,
                                                   Individual* child2) {
                                           
    for (int bit=0; bit<num_nodes; bit++) {
        int crossover_decision = urandint(100);
        if (crossover_decision < PUC_PROB*100) {
            // exchange bits:
            putbit(child1->partition, 
                   bit, 
                   getbit(pop[parent_idxs[1]].partition, bit)
                  );
            putbit(child2->partition,
                   bit,
                   getbit(pop[parent_idxs[0]].partition, bit)
                  );

        }
        else {
            // just copy the bit to the respective child:
            putbit(child1->partition,
                   bit,
                   getbit(pop[parent_idxs[0]].partition, bit)
                  );
            putbit(child2->partition,
                   bit,
                   getbit(pop[parent_idxs[1]].partition, bit)
                  );
        }
    }
}


#endif /* _CROSSOVER_H_ */ 

