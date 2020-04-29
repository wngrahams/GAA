/*
 * island.h
 *
 * file defining islands and associated functions
 */

#ifndef _ISLAND_H_
#define _ISLAND_H_

#include "ga-params.h"
#include "ga-utils.h"
#include "llist/llist.h"


typedef struct Island {
    List* member_list;        // pointer to linked list containing the indices 
                              // of all members of this island
    int num_members;          // number of members of this island
    double avg_fitness;       // average fitness of the members of this island
    double* migration_probs;  // array containing probabilities of migration
                              // to other islands (by index), including itself
} Island;

/*
 * calculates the average fitness of the individuals on the given island
 * and stores the value in the island's avg_fitness member.
 */
static inline void calc_island_fitness(Island* isl, Individual* pop) {

    double avg_fitness = 0.0;
    Lnode* member = (isl->member_list)->head;
    for (int j=0; j<isl->num_members; j++) {

        int indiv_fitness = pop[*(int*)(member->data)].fitness;
        avg_fitness += (double)indiv_fitness/isl->num_members;

        member = member->next;
    }
    isl->avg_fitness = avg_fitness;
}

/*
 * Initializes islands.
 * If init probs is NULL, the migration probabilites are initialized based on 
 * the parameters in ga-params.h
 * Otherwise, the probabilites are copied from the passed array
 */
static inline void init_island(Island* isl, 
                               int islno,
                               List* init_list, 
                               double* init_probs) {

    isl->member_list = init_list;

    isl->num_members = 0;
    Lnode* curr = isl->member_list->head;
    while (curr) {
        isl->num_members++;
        curr = curr->next;
    }

    isl->migration_probs = malloc(NUM_ISLANDS * sizeof(double));
    CHECK_MALLOC_ERR(isl->migration_probs);

    if (NULL == init_probs) {
        // assign initial migration probabilities        
        for (int i=0; i<NUM_ISLANDS; i++) {
            if (i!=islno) {
                isl->migration_probs[i] = PROB_ISLAND_MIGRATE;
            }
            else {                    
                isl->migration_probs[i] = PROB_ISLAND_STAY;
            }
        }
    }
    else {
        for (int i=0; i<NUM_ISLANDS; i++) {
            isl->migration_probs[i] = init_probs[i];
        }
    }

} /* END init_island */

/*
 * Initializes a group of islands with the population evenly distributed among
 * them
 */
static inline void init_archipelago(Island* arch, Individual* pop) {
    
    List** member_lists = malloc(NUM_ISLANDS * sizeof(List));
    CHECK_MALLOC_ERR(member_lists);
    for (int i=0; i<NUM_ISLANDS; i++) {
        member_lists[i] = malloc(POP_SIZE * sizeof(List));
        CHECK_MALLOC_ERR(member_lists[i]);
        initList(member_lists[i]);
    }

    // evenly distribute the population
    for (int i=0; i<POP_SIZE; i++) {
        int* idx_to_add = malloc(sizeof(int));
        CHECK_MALLOC_ERR(idx_to_add);
        *idx_to_add = i;
        addBack(member_lists[i%NUM_ISLANDS], idx_to_add);
    }

    // initilize the islands with default migration probabilities
    // then assign the average fitness for each
    for (int i=0; i<NUM_ISLANDS; i++) {
        init_island(&(arch[i]), i, member_lists[i], NULL);
        calc_island_fitness(&(arch[i]), pop);
    }

    free(member_lists);

} /* END init_archipelago */


#endif /* _ISLAND_H_ */

