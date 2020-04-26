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

#include "bitarray.h"
#include "crossover.h"
#include "GAA.h"
#include "ga-params.h"
#include "ga-utils.h"
#include "graph-parser.h"
#include "selection.h"


int main(int argc, char** argv) {

    Graph* graph;
    struct timespec total_start, total_stop, 
                    selection_start, selection_stop,
                    crossover_start, crossover_stop,
                    mutation_start, mutation_stop,
                    fitness_start, fitness_stop,
                    diversity_start, diversity_stop;
    double total_time = 0;
    double selection_time = 0;
    double crossover_time = 0;
    double mutation_time = 0;
    double fitness_time = 0;
    double diversity_time = 0;

    if (argc != 2) {
        fprintf(stderr, "%s\n", "usage: gaa <graph_file>");
        exit(1);
    }

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &total_start);

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
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &fitness_start);
        population[i].fitness = calc_fitness(graph, &(population[i]));
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &fitness_stop);
        fitness_time += (fitness_stop.tv_sec - fitness_start.tv_sec) + 
                 (fitness_stop.tv_nsec - fitness_start.tv_nsec)/1e9;

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

        double diversity = 0;

        if (gen == 0)
            printf("Starting GA for %d generations...\n", NUM_OF_GENERATIONS);
        else if (gen%100==0) {
            clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &diversity_start);
            diversity = calc_diversity(population, graph->v);
            clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &diversity_stop);
            diversity_time += (diversity_stop.tv_sec - diversity_start.tv_sec) + 
                 (diversity_stop.tv_nsec - diversity_start.tv_nsec)/1e9;
            printf("\r%d generations complete... Diversity=%.4f", 
                   gen, 
                   diversity
                  );
            fflush(stdout);
        }
        
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
            // Select a pair of parent chromosomes from the current population.
            int parent_idxs[2];
            clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &selection_start);
            /*
            roulette_wheel_selection(population, 
                                     parent_idxs, 
                                     total_inverse_fitness
                                    );
            */
            parent_idxs[0] = tournament_selection(population);
            parent_idxs[1] = tournament_selection(population);

            clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &selection_stop);
            selection_time += (selection_stop.tv_sec - selection_start.tv_sec) + 
                            (selection_stop.tv_nsec - selection_start.tv_nsec)/1e9;
            
            /*
            printf("selected parents %d and %d.\n",
                   parent_idxs[0],
                   parent_idxs[1]
                  );
            */
            
            // CROSSOVER:
            clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &crossover_start);
            /*
            single_point_crossover(population, 
                                   parent_idxs, 
                                   graph->v,
                                   &(children[ i ]),
                                   &(children[i+1])
                                  );*/
            /*            
            two_point_crossover(population,
                                parent_idxs,
                                graph->v,
                                &(children[ i ]),
                                &(children[i+1])
                               );*/
            
            parameterized_uniform_crossover(population,
                                            parent_idxs,
                                            graph->v,
                                            &(children[ i ]),
                                            &(children[i+1])
                                           );
            clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &crossover_stop);
            crossover_time += (crossover_stop.tv_sec - crossover_start.tv_sec) + 
                 (crossover_stop.tv_nsec - crossover_start.tv_nsec)/1e9;
              
            // MUTATION:
            // Mutate the two offspring at each locus with probability 
            // MUTATION_RATE (the mutation probability or mutation rate)
            
            //printf("Mutation...\n");
            clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &mutation_start);
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
            clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &mutation_stop);
            mutation_time += (mutation_stop.tv_sec - mutation_start.tv_sec) + 
                 (mutation_stop.tv_nsec - mutation_start.tv_nsec)/1e9;

            // CALCULATE FITNESS OF NEW CHILDREN
            // TODO: move this outside of the loop so that the hardware can do
            // them all in parallel
            clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &fitness_start);
            children[ i ].fitness = calc_fitness(graph, &(children[ i ]));
            children[i+1].fitness = calc_fitness(graph, &(children[i+1]));
            clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &fitness_stop);
            fitness_time += (fitness_stop.tv_sec - fitness_start.tv_sec) + 
                 (fitness_stop.tv_nsec - fitness_start.tv_nsec)/1e9;
            
        } /* END loop for one generation: SELECTION, CROSSOVER, MUTATION */

        // Replace the current population with the new population
        // TODO: instead of making deep copies, freeing the children, and then
        // reallocating, just move the pointers around
        total_inverse_fitness = 0;
        for (int i=0; i<POP_SIZE; i++) {
            for (int j=0; j<RESERVE_BITS(graph->v); j++) {
                population[i].partition[j] = children[i].partition[j];
            }
            population[i].fitness = children[i].fitness;
            total_inverse_fitness += 1.0/(double)population[i].fitness;
        }

        // TODO:
        // Put hardware fitness here (all individuals in parallel)

        // free the children
        for (int i=0; i<POP_SIZE; i++) {
            free(children[i].partition);
        }
        free(children);

    } // end of evolution loop
    printf("\r%d generations complete.  \n", NUM_OF_GENERATIONS);

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
    int p0_cnt = 0;
    int p1_cnt = 0;
    for (int i=0; i<graph->v; i++) {
        if (getbit(population[min_idx].partition, i) == 0)
            p0_cnt++;
        else
            p1_cnt++;

        //printf("%d", getbit(population[min_idx].partition, i));        
    }
    printf("\n");
    int external_cost = 0;
    for (int i=0; i<graph->e; i++) {
        if (getbit(population[min_idx].partition, (graph->edges)[i]->n1) !=
            getbit(population[min_idx].partition, (graph->edges)[i]->n2)   ) {
            
            external_cost += (graph->edges)[i]->weight;
        }
    }
    printf("\tFitness = %d\n", population[min_idx].fitness);
    printf("\tNumber of nodes in partition 0: %d\n", p0_cnt);
    printf("\t                             1: %d\n", p1_cnt);
    printf("\tTotal external cost: %d\n", external_cost);
    printf("\n");

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &total_stop);
    total_time = (total_stop.tv_sec - total_start.tv_sec) + 
                 (total_stop.tv_nsec - total_start.tv_nsec)/1e9;
    printf("Timing info:\n");
    printf("\tTotal elapsed time:      %8.2f sec\n", total_time);
    printf("\tTime spent in selection: %8.2f sec (%4.1f%%)\n", 
           selection_time, 
           (selection_time/total_time)*100
          );
    printf("\tTime spent in crossover: %8.2f sec (%4.1f%%)\n", 
           crossover_time,
           (crossover_time/total_time)*100
          );
    printf("\tTime spent in mutation:  %8.2f sec (%4.1f%%)\n", 
           mutation_time,
           (mutation_time/total_time)*100
          );
    printf("\tTime spent in fitness:   %8.2f sec (%4.1f%%)\n", 
           fitness_time,
           (fitness_time/total_time)*100
          );
    printf("\tTime spent in diversity: %8.2f sec (%4.1f%%)\n", 
           diversity_time,
           (diversity_time/total_time)*100
          );

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
 * Calculates the diversity of the population by finding the hamming distance
 * between each individual's partition. O(POP_SIZE^2) runtime
 * The return value is the average percentage of different bits between any 
 * two given individuals in the population.
 */
double calc_diversity(Individual* pop, int num_nodes) {
    int hdist;
    double diversity = 0;

    for (int i=0; i<POP_SIZE; i++) {
        for (int j=0; j<POP_SIZE; j++) {

            hdist = 0;
            if (i != j) {
                /*
                printf("Individual %d:\n", i);
                printf("\tpartition: ");
                for( int m=0; m<num_nodes; m++) {
                    printf("%d", getbit(pop[i].partition, m));
                }
                printf("\n");
                printf("Individual %d:\n", j);
                printf("\tpartition: ");
                for( int m=0; m<num_nodes; m++) {
                    printf("%d", getbit(pop[j].partition, m));
                }
                printf("\n");
                */

                for (int k=0; k<RESERVE_BITS(num_nodes); k++) {
                    hdist += hamming_distance(pop[i].partition[k],
                                              pop[j].partition[k]
                                             );
                }
                //printf("hamming distance: %d\n", hdist);
            }

            diversity += (double)hdist/num_nodes;
        }
    }

    return diversity/(POP_SIZE*(POP_SIZE-1));
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

