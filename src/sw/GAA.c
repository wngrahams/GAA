/*
 * GAA.c
 *
 * Graph partitioning using a genetic algorithm with hardware acceleration
 *
 */

#include <assert.h>  // assert
#include <limits.h>  // INT_MAX
#include <stdio.h>   // printf
#include <stdlib.h>  // malloc
#include <time.h>    // time

#include "GAA.h"
#include "ga-params.h"
#include "ga-utils.h"
#include "graph-parser.h"


int main(int argc, char** argv) {

    Graph* graph;
 
    if (argc != 2) {
        fprintf(stderr, "%s\n", "usage: gaa <graph_file>");
        exit(1);
    }

    // allocate memory for a graph struct
    graph = malloc(sizeof(Graph));
    CHECK_MALLOC_ERR(graph);

    // parse graph from file specified on command line
    if (!parse_graph_from_file(argv[1], graph)) {
        goto cleanup_graph;
    }
    
    // seed random number generator
    srand(time(0));
    
    double total_inverse_fitness = 0;  // used in selection to select 
                                       // individuals with probability 
                                       // inversely proportional to their
                                       // fitness (since we are looking to
                                       // minimize fitness)

    // initialize population
    if (!POP_SIZE % 2) {
        fprintf(stderr, "POPULATION SIZE MUST BE AN EVEN NUMBER");
        goto cleanup_graph_contents;
    }
    Individual* population = malloc(POP_SIZE * sizeof(Individual));
    CHECK_MALLOC_ERR(population);
    
    for (int i=0; i<POP_SIZE; i++) {
        // allocate memory for the individual's partition
        population[i].partition = 
                malloc(RESERVE_BITS(graph->v) * sizeof(bitarray_t));
        CHECK_MALLOC_ERR(population[i].partition);

        // initialize partitions to 0:
        for (int j=0; j<RESERVE_BITS(graph->v); j++) {
            (population[i].partition)[j] = 0;
        }

        // create a random starting partition: 
        // TODO: use a better random number generation method (ie for a
        // more uniform distribution)
        for (int j=0; j<graph->v; j++) {
            int rand_bit = rand() % 2;
            putbit(population[i].partition, j, rand_bit); 
        }

        // calculate fitness:
        
        // TODO: take fitness calculation out of loop so that hardware can do
        // do it all in parallel
        population[i].fitness = calc_fitness(graph, &(population[i]));
        total_inverse_fitness += 1.0/(double)population[i].fitness;
        

        /*
        printf("Individual %d:\n", i);
        printf("\tpartition: ");
        for( int j=0; j<graph->v; j++) {
            printf("%d", getbit(population[i].partition, j));
        }
        printf("\n");
        printf("\tfitness = %d\n", population[i].fitness);
        */

    } /* END initialize population */

    // TODO: calculate initial fitnesses in hardware here
    //  ---------

    /*
    printf("Total inverse fitness: %f\n", total_inverse_fitness);
    printf("RAND_MAX: %d\n", RAND_MAX);
    */

    // evolutionary loop
    for (int gen=0; gen<NUM_OF_GENERATIONS; gen++) {
        
        // initialize child population
        Individual* children = malloc(POP_SIZE * sizeof(Individual));
        CHECK_MALLOC_ERR(children);

        for (int i=0; i<POP_SIZE; i+=2) {

            children[i].partition = 
                    malloc(RESERVE_BITS(graph->v) * sizeof(bitarray_t));
            CHECK_MALLOC_ERR(children[i].partition);
            children[i+1].partition = 
                    malloc(RESERVE_BITS(graph->v) * sizeof(bitarray_t));
            CHECK_MALLOC_ERR(children[i].partition);

            // SELECTION:
            // Select a pair of parent chromosomes from the current population,
            // the probability of selection being an increasing function of 
            // fitness. Selection is done "with replacement," meaning that the 
            // same chromosome can be selected more than once to become a 
            // parent.
            int parent_idxs[2] = {-1, -1};
            for (int j=0; j<2; j++) {

                do {
                    // inverse fitness is used so that an individual with a 
                    // value of fitness closer to 0 is more likely to be 
                    // selected
                    double rand_selection = total_inverse_fitness 
                                            * ((double)rand()/RAND_MAX);
                    double fitness_cnt = 0.0;
                    for (int k=0; k<POP_SIZE; k++) {
                        parent_idxs[j] = k;
                        fitness_cnt += 1.0/(double)population[i].fitness;
                        if (fitness_cnt >= rand_selection) 
                            break;
                    }
                } while (j == 1 && parent_idxs[0] == parent_idxs[1]);
                // this makes sure the second parent idx is not the same as the
                // first

            } /* END SELECTION */

            /*
            printf("selected parents %d and %d.\n",
                   parent_idxs[0],
                   parent_idxs[1]
                  );
            */

            // CROSSOVER:
            // With probability CROSSOVER_PROB (the "crossover probability" or 
            // "crossover rate"), cross over the pair at a randomly chosen 
            // point (chosen with uniform probability) (TODO) to form two 
            // offspring. If no crossover takes place, form two offspring that 
            // are exact copies of their respective parents.

            double crossover_decision = (double)rand()/RAND_MAX;
            if (crossover_decision < CROSSOVER_PROB) {

                //printf("Performing crossover... ");

                // single point crossover -- TODO: change to multiple point, 
                //     where crossover rate for a pair of parents is the number
                //     of points at which a crossover takes place.

                // choose the bit at which to crossover: (TODO better random)
                // doesn't pick bit 0 because that is the same as no crossover
                int crossover_point = (rand()%(graph->v-1)) + 1;  // (0, graph->v)

                /*
                printf("Crossover point = %d\n", crossover_point);

                printf("\tParents: ");
                for( int j=0; j<graph->v; j++) {
                    printf("%d", getbit(population[parent_idxs[0]].partition, j));
                }
                printf(", ");
                for( int j=0; j<graph->v; j++) {
                    printf("%d", getbit(population[parent_idxs[1]].partition, j));
                }
                printf("\n");
                */

                // fill in the non-crossover part of the bitarray as a copy of 
                // the parents:
                for (int bit_to_copy=0;
                         bit_to_copy<crossover_point;
                         bit_to_copy++) {
                    
                    putbit(children[i].partition,
                           bit_to_copy,
                           getbit(population[parent_idxs[0]].partition, 
                                  bit_to_copy
                                 )
                          );
                    putbit(children[i+1].partition,
                           bit_to_copy,
                           getbit(population[parent_idxs[1]].partition, 
                                  bit_to_copy
                                 )
                          );
                }


                // do the crossover:
                for (int bit_to_swap=crossover_point; 
                         bit_to_swap<graph->v; 
                         bit_to_swap++) {

                    putbit(children[i].partition, 
                           bit_to_swap, 
                           getbit(population[parent_idxs[1]].partition, 
                                  bit_to_swap
                                 )
                          );
                    putbit(children[i+1].partition,
                           bit_to_swap,
                           getbit(population[parent_idxs[0]].partition, 
                                  bit_to_swap
                                 )
                          );
                }

                /*
                printf("\tChildren: ");
                for (int j=0; j<graph->v; j++) {
                    printf("%d", getbit(children[i].partition, j));
                }
                printf(", ");
                for (int j=0; j<graph->v; j++) {
                    printf("%d", getbit(children[i+1].partition, j));
                }
                printf("\n");
                */

            }
            else {

                //printf("No crossover.. children will be copies of parents.\n");
                // the two children are exact copies of the parents
                for (int j=0; j<RESERVE_BITS(graph->v); j++) {
                    children[ i ].partition[j] = 
                            population[parent_idxs[0]].partition[j];
                    children[i+1].partition[j] = 
                            population[parent_idxs[1]].partition[j];
                }       

            } /* END CROSSOVER */

            // MUTATION:
            // Mutate the two offspring at each locus with probability 
            // MUTATION_RATE (the mutation probability or mutation rate)
            
            //printf("Mutation...\n");
            for (int childno=0; childno<2; childno++) {
                
                //printf("\tMutating child %d at bits: ", childno);

                for (int locus=0; locus<graph->v; locus++) {
                    
                    double mutation_decision1 = (double)rand()/RAND_MAX;
                    if (mutation_decision1 < MUTATION_PROB) {

                        //printf("%d, ", locus);
                        
                        // mutate: 1->0 or 0->1
                        putbit(children[i+childno].partition,
                               locus,
                               !getbit(children[i+childno].partition, locus)
                              );
                    }
                }
                /*
                printf("\n");
                printf("\tResult: ");
                for (int j=0; j<graph->v; j++) {
                    printf("%d", getbit(children[i+childno].partition, j));
                }
                printf("\n");
                */

            } /* END MUTATION */

            // CALCULATE FITNESS OF NEW CHILDREN
            // TODO: move this outside of the loop so that the hardware can do
            // them all in parallel
            children[ i ].fitness = calc_fitness(graph, &(children[ i ]));
            children[i+1].fitness = calc_fitness(graph, &(children[i+1]));
            
        } /* END loop for one generation: SELECTION, CROSSOVER, MUTATION */

        // Replace the current population with the new population
        // TODO: instead of making deep copies, freeing the children, and then
        // reallocating, just move the pointers around
        for (int i=0; i<POP_SIZE; i++) {
            for (int j=0; j<RESERVE_BITS(graph->v); j++) {
                population[i].partition[j] = children[i].partition[j];
            }
            population[i].fitness = children[i].fitness;
        }

        // TODO:
        // Put hardware fitness here (all individuals in parallel)

        // free the children
        for (int i=0; i<POP_SIZE; i++) {
            free(children[i].partition);
        }
        free(children);

    } // end of evolution loop

    /*
    printf("All individuals:\n");
    for (int i=0; i<POP_SIZE; i++) {
        printf("Individual %d:\n", i);
        printf("\tpartition: ");
        for( int j=0; j<graph->v; j++) {
            printf("%d", getbit(population[i].partition, j));
        }
        printf("\n");
        printf("\tfitness = %d\n", population[i].fitness);
    }*/

    // print best individual
    int min_fitness = INT_MAX;
    int min_idx = -1;
    for (int i=0; i<POP_SIZE; i++) {
        if (population[i].fitness < min_fitness) {
            min_fitness = population[i].fitness;
            min_idx = i;
        }
    }
    printf("Most fit individual: ");
    for (int i=0; i<graph->v; i++) {
        printf("%d", getbit(population[min_idx].partition, i));        
    }
    printf("\n");
    printf("\tFitness = %d\n", population[min_idx].fitness);

    // free population
    for (int i=0; i<POP_SIZE; i++) {
        free(population[i].partition);
    }
    free(population);

cleanup_graph_contents:
    // free memory used for graph:
    for (int i=0; i<graph->v; i++) {
        free((graph->nodes)[i]);
    }
    free(graph->nodes);
    for (int i=0; i<graph->e; i++) {
        free((graph->edges)[i]);
    }
    free(graph->edges);
cleanup_graph:
    free(graph);

    return 0;
}

/*
 * Calculates fitness of an individual with respect to the associated graph.
 * An individual with a better partition will have a fitness value closer to 0.
 */
int calc_fitness(Graph* graph, Individual* indiv) {

    int fitness = 0;  
    
    // for each edge in the graph, add the edge weight to the fitness if the 
    // two nodes are in different partitions
    for (int i=0; i<graph->e; i++) {
        //printf("node 1: %d ", (graph->edges)[i]->n1);
        //printf("node 2: %d ", (graph->edges)[i]->n2);
        //printf("p1: %d ", getbit(indiv->partition, (graph->edges)[i]->n1));
        //printf("p2: %d\n", getbit(indiv->partition, (graph->edges)[i]->n2));
        if (getbit(indiv->partition, (graph->edges)[i]->n1) !=
            getbit(indiv->partition, (graph->edges)[i]->n2)   ) {
            
            fitness += (graph->edges)[i]->weight;
        }
    }

    // to make lopsided partitions costly, add 
    // abs|sum of node weights in partition 1 - 
    //          sum of node weights in partition 2|
    // to the fitness
    int p0_weight = 0;
    int p1_weight = 0;

    for (int i=0; i<graph->v; i++) {
        int node_weight = (graph->nodes)[i]->weight;
        if (getbit(indiv->partition, i) == 0)
            p0_weight += node_weight;
        else if (getbit(indiv->partition, i) == 1)
            p1_weight += node_weight;
        else
            assert(0);  // oh no
    }

    if (p1_weight > p0_weight)
        fitness += (p1_weight - p0_weight);
    else
        fitness += (p0_weight - p1_weight);

    return fitness;

}

/*
 * Shuffes the array of integers passed to the function
 * Citation: benpfaff.org/writings/clc/shuffle.html
 */
void shuffle(int *arr, int n) {
    if (n > 1) {
        int i;
        for (i = 0; i < n - 1; i++) {
            int j = i + rand() / (RAND_MAX / (n - i) + 1);
            int t = arr[j];
            arr[j] = arr[i];
            arr[i] = t;
        }
    }
}

