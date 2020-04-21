/*
 * selection.h
 *
 * Contains multiple different selection algorithms to be used in GAA.c
 *
 */

#ifndef _SELECTION_H_
#define _SELECTION_H_

#include "ga-params.h"


/*
 * Roulette Wheel Selection: two parents are selected from the current 
 * population, with the probability of selection being an increasing fucntion
 * of inverse fitness (a decreasing function of fitness, since less fit 
 * individuals have a higher fitness score). 
 * The indices of the two chosen parents are placed in the passed array.
 */
static inline void roulette_wheel_selection(Individual* pop, 
                                            int selected_parents[],
                                            double total_inverse_fitness) {

	selected_parents[0] = -1;
	selected_parents[1] = -1;
    for (int parent_idx=0; parent_idx<2; parent_idx++) {

    	do {
        	// inverse fitness is used so that an individual with a 
        	// value of fitness closer to 0 is more likely to be 
            // selected
            double rand_selection = total_inverse_fitness 
									* ((double)rand()/RAND_MAX);
            double fitness_cnt = 0.0;
            for (int i=0; i<POP_SIZE; i++) {
            	selected_parents[parent_idx] = i;
                fitness_cnt += 1.0/(double)pop[i].fitness;
                if (fitness_cnt >= rand_selection) 
                	break;
            }
        } while (parent_idx == 1 && selected_parents[0]==selected_parents[1]);
        // this makes sure the second parent idx is not the same as the
        // first
    }
}

#endif /* _SELECTION_H_ */

