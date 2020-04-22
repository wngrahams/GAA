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

/*
 * Tournament Selection: Two individuals are chosen at random from the 
 * population. A random number r is then chosen between 0 and 1. If r < k 
 * (where k is a parameter, for example 0.75), the fitter of the two 
 * individuals is selected to be a parent; otherwise the less fit individual 
 * is selected. The two are then returned to the original population and can 
 * be selected again.
 */
static inline int tournament_selection(Individual* pop) {
    
    int parent1_idx = rand() % POP_SIZE;
    int parent2_idx = -1;
    do {
        parent2_idx = rand() % POP_SIZE;
    } while (parent2_idx == parent1_idx);  // ensures the parents are diffent
                                           // individuals
                                           
    double r = (double)rand()/RAND_MAX;
    if (r < TOURNAMENT_SELECT_PROB) {
        // select fitter individual (individual with lower fitness score)
        if (pop[parent1_idx].fitness < pop[parent2_idx].fitness)
            return parent1_idx;
        else
            return parent2_idx;
    }
    else {
        // select less fit individual (individual with higher fitness score)_
        if (pop[parent1_idx].fitness < pop[parent2_idx].fitness)
            return parent2_idx;
        else
            return parent1_idx;
    }
}

#endif /* _SELECTION_H_ */

