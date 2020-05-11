/*
 * GAA.c
 *
 * Graph partitioning using a genetic algorithm with hardware acceleration
 *
 */

#define _POSIX_C_SOURCE 199309L

#include <assert.h>     // assert
#include <fcntl.h>
#include <limits.h>     // INT_MAX
#include <stdio.h>      // printf
#include <stdlib.h>     // malloc
#include <string.h>     // memset
#include <sys/ioctl.h>  // ioctl
#include <sys/mman.h>   // mmap
#include <sys/types.h>
#include <sys/stat.h>   
#include <time.h>       // time
#include <unistd.h>

#include "bitarray.h"
#include "crossover.h"
#include "GAA.h"
#include "ga-params.h"
#include "ga-utils.h"
#include "gaa_fitness_driver.h"
#include "graph-parser.h"
#include "mergesort.h"
#include "selection.h"

#define SDRAM_ADDR 0xC0000000
#define SDRAM_SPAN 0x04000000  // 64 MB of SDRAM from 0xC0000000 to 0xC3FFFFFF

#define MAX_NUM_EDGES (SDRAM_SPAN/(sizeof(uint32_t)*2))
#define MAX_NUM_NODES (uint32_t)0xFFFFFFFF


int main(int argc, char** argv) {

    Graph* graph;
    struct timespec total_start, total_stop, 
                    selection_start, selection_stop,
                    crossover_start, crossover_stop,
                    mutation_start, mutation_stop,
                    fitness_start, fitness_stop,
                    diversity_start, diversity_stop,
                    migration_start, migration_stop;
    double total_time = 0;
    double selection_time = 0;
    double crossover_time = 0;
    double mutation_time = 0;
    double fitness_time = 0;
    double diversity_time = 0;
    double migration_time = 0;

    int migration_count;

    int mmap_fd;                          // file descriptor for /dev/mem
    void* sdram_mem;                      // void* pointer to base of sdram

    volatile uint32_t *sdram_ptr = NULL;  // pointer to data in sdram
                                          // We store the edges as two 32-bit 
                                          // nodes, so this indexes the 
                                          // individual nodes of an edge.
                                          // Must be volatile, since the data 
                                          // in the sdram may change outside of
                                          // this userspace program.

    if (argc != 2) {
        fprintf(stderr, "%s\n", "usage: gaa <graph_file>");
        exit(1);
    }

    // TEST DRIVER
    int gaa_fitness_fd;
    gaa_fitness_arg_t gaa_arg;
    uint8_t _i, _j, out;

    static const char filename[] = "/dev/gaa_fitness";

    static const gaa_fitness_inputs_t initial_inputs = { 0x0F , 0xF0 };

    gaa_fitness_inputs_t current_inputs = {.p1 = initial_inputs.p1,
                                         .p2 = initial_inputs.p2};

    printf("GAA Fitness test beginning\n");

    if ( (gaa_fitness_fd = open(filename, O_RDWR)) == -1) {
        fprintf(stderr, "could not open %s\n", filename);
        return -1;
    }

    set_input_values(&initial_inputs, gaa_fitness_fd);
    out = print_output_value(gaa_fitness_fd);
    assert(out == 0xFF);

    for (_i=0x00, _j=0x0F; _i<=0x0F && _j>=0x00; _i++, _j--) {
        current_inputs.p1 = _i;
        current_inputs.p2 = _j;
        set_input_values(&current_inputs, gaa_fitness_fd);
        out = print_output_value(gaa_fitness_fd);
        //usleep(500000);
    }

    printf("Testing mmap of FPGA SDRAM:\n");

    // open memory with uncached access:
    if((mmap_fd = open("/dev/mem", (O_RDWR | O_SYNC))) == -1) {
        printf("ERROR: could not open \"/dev/mem\"...\n");
        exit(1);
    }
    // mmap(addr, length, prot, flags, fd, offset);
    sdram_mem = mmap(0, SDRAM_SPAN, (PROT_READ|PROT_WRITE), MAP_SHARED, 
                     mmap_fd, SDRAM_ADDR);
    if (MAP_FAILED == sdram_mem) {
        fprintf(stderr, "MMAP FAILED\n");
        exit(1);
    }

    // mmap file descriptor is no longer needed and can be closed
    close(mmap_fd);

    sdram_ptr = (uint32_t*)(sdram_mem);

    printf("Hardware Constraints:\n");
    printf("\tMax number of edges: %u\n", MAX_NUM_EDGES);
    printf("\tMax number of nodes: %u\n", MAX_NUM_NODES);

    // clear sdram:
    // (each egde in hardware is two 32 bit node indices)
    printf("Clearing SDRAM... ");
    for (int i=0; i<MAX_NUM_EDGES*2; i++) {
        *(sdram_ptr + i) = (uint32_t)0; 
    }
    printf("Done.\n");
    
    // write to sdram:
    printf("Writing to SDRAM:\n");
    for (uint32_t i=0; i<5; i++) {
        printf("\tAddr: %p Value: %d\n", (sdram_ptr + i), i);
        *(sdram_ptr + i) = i;
    }

    // read from sdram:
    printf("Reading from SDRAM:\n");
    for (int i=0; i<5; i++) {
        printf("\tAddr: %p Value: %d\n", (sdram_ptr + i), *(sdram_ptr + i));
    }

    // unmap mapped memory
    munmap(sdram_mem, SDRAM_SPAN);

    printf("GAA Fitness test finished\n");

    // END TEST

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &total_start);

    // allocate memory for a graph struct
    graph = malloc(sizeof(Graph));
    CHECK_MALLOC_ERR(graph);

    // parse graph from file specified on command line
    if (!parse_graph_from_file(argv[1], graph)) {
        goto cleanup_graph;
    }

    if (graph->v > MAX_NUM_NODES || graph->e > MAX_NUM_EDGES) {
        printf("Graph size is not within the constraints of the hardware.");
        printf(" Exiting...\n");
        goto cleanup_graph;
    }
    
    // seed random number generator
    srand(time(0));
    
    double total_inverse_fitness = 0;  // used in selection to select 
                                       // individuals with probability 
                                       // inversely proportional to their
                                       // fitness (since we are looking to
                                       // minimize fitness)

    // initialize islands and their populations
    if (!POP_SIZE % 2) {
        fprintf(stderr, "POPULATION SIZE MUST BE AN EVEN NUMBER");
        goto cleanup_graph_contents;
    }

    // array of islands, on each island is a population
    Individual* archipelago[NUM_ISLANDS];

    for (int isl=0; isl<NUM_ISLANDS; isl++) {
        Individual* population = malloc(POP_SIZE * sizeof(Individual));
        CHECK_MALLOC_ERR(population);

        init_population(population, graph->v);

        archipelago[isl] = population;
    }

    // calculate initial fitness for each individual on each island
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &fitness_start);
    for (int isl=0; isl<NUM_ISLANDS; isl++) {
        for (int idv=0; idv<POP_SIZE; idv++) {

            archipelago[isl][idv].fitness = 
                    calc_fitness(graph, &(archipelago[isl][idv]));

            total_inverse_fitness += 1.0/(double)archipelago[isl][idv].fitness;
        }
    }

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &fitness_stop);
    fitness_time += (fitness_stop.tv_sec - fitness_start.tv_sec) + 
                 (fitness_stop.tv_nsec - fitness_start.tv_nsec)/1e9;

    // allocate and initialize memory needed for migration
    int** sorted_indices_list = malloc(NUM_ISLANDS * sizeof(int*));
    CHECK_MALLOC_ERR(sorted_indices_list);
    for (int i=0; i<NUM_ISLANDS; i++) {
        sorted_indices_list[i] = malloc(POP_SIZE * sizeof(int));
        CHECK_MALLOC_ERR(sorted_indices_list[i]);
        for (int j=0; j<POP_SIZE; j++) {
            sorted_indices_list[i][j] = j;
        }
    }
    
    /* EVOLUTIONARY LOOP */
    for (int gen=0; gen<NUM_GENERATIONS; gen++) {

        double diversity = 0;

        if (gen == 0)
            printf("Starting GA for %d generations...\n", NUM_GENERATIONS);
        else if (gen%DIVERSITY_PERIOD == 0) {

            printf("\r%d generations complete... Diversity on each island: ", gen);

            clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &diversity_start);

            for (int isl=0; isl<NUM_ISLANDS; isl++) {
                diversity = calc_diversity(archipelago[isl], graph->v);
                printf("%.2f", diversity);
                if (isl < NUM_ISLANDS-1)
                    printf(", ");
            }

            clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &diversity_stop);
            diversity_time += (diversity_stop.tv_sec - diversity_start.tv_sec) + 
                 (diversity_stop.tv_nsec - diversity_start.tv_nsec)/1e9;
            
            fflush(stdout);
        }

        /* MIGRATION */
        if (gen == 0) {
            migration_count = 1;
            assert(NUM_TO_MIGRATE <= POP_SIZE/2);
        }
        else if (gen%MIGRATION_PERIOD == 0) {

            clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &migration_start);

            if (migration_count%NUM_ISLANDS == 0) {
                migration_count = 1;
            }

            // sort the indicies for each island by fitness
            for (int isl=0; isl<NUM_ISLANDS; isl++) {
                mergesort_idv(sorted_indices_list[isl], 
                              0, 
                              POP_SIZE-1, 
                              archipelago[isl]
                             );
            }

            for (int isl=0; isl<NUM_ISLANDS; isl++) {

                // migrate a top percentage of individuals migration_count 
                // islands over from the current island, replacing the same
                // worst percentage (ie top 5% replaces bottom 5%)
                for (int idv=0; idv<NUM_TO_MIGRATE; idv++) {
                    //printf("sender_isl: %d\n", isl);
                    int recipient_isl = (isl + migration_count)%NUM_ISLANDS;
                    //printf("recipient_isl: %d\n", recipient_isl);
                    int idx_to_send = sorted_indices_list[isl][idv];
                    //printf("idx_to_send: %d\n", idx_to_send);
                    int idx_to_replace = 
                            sorted_indices_list[recipient_isl][POP_SIZE-1-idv];
                    /*printf(
"Sending idv %3d on island %3d to replace idv %3d on island %3d\n", 
                                    idx_to_send, 
                                    isl, idx_to_replace, recipient_isl);
                    */
        
                    // do the copy:
                    archipelago[recipient_isl][idx_to_replace].fitness
                            = archipelago[isl][idx_to_send].fitness;
                    for (int i=0; i<RESERVE_BITS(graph->v); i++) {
                        archipelago[recipient_isl][idx_to_replace].partition[i]
                                = archipelago[isl][idx_to_send].partition[i];
                    }
                }
                //printf("\n");
                
            }
            //printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");

            migration_count++;

            clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &migration_stop);
            migration_time += (migration_stop.tv_sec - migration_start.tv_sec) + 
                    (migration_stop.tv_nsec - migration_start.tv_nsec)/1e9;

        } /* END MIGRATION */

        // loop over each island
        for (int isl=0; isl<NUM_ISLANDS; isl++) {

            // Initialize children
            Individual* children = malloc(POP_SIZE * sizeof(Individual));
            CHECK_MALLOC_ERR(children);

            // create child population two individuals at a time using the 
            // genetic operators of selection, crossover, and mutation
            for (int idv=0; idv<POP_SIZE; idv+=2) {
    
                // initialize memory for child partitions
                children[idv].partition = 
                        malloc(RESERVE_BITS(graph->v) * sizeof(bitarray_t));
                CHECK_MALLOC_ERR(children[idv].partition);
                memset(children[idv].partition,
                       0,
                       RESERVE_BITS(graph->v)*sizeof(bitarray_t)
                      );
                children[idv+1].partition = 
                        malloc(RESERVE_BITS(graph->v) * sizeof(bitarray_t));
                CHECK_MALLOC_ERR(children[idv+1].partition);
                memset(children[idv+1].partition, 
                       0, 
                       RESERVE_BITS(graph->v)*sizeof(bitarray_t)
                      );

                /* SELECTION */
                int parent_idxs[2] = {-1, -1};
                clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &selection_start);

                parent_idxs[0] = tournament_selection(archipelago[isl]);
                do {
                    parent_idxs[1] = tournament_selection(archipelago[isl]);
                } while (parent_idxs[0] == parent_idxs[1]);

                clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &selection_stop);
                selection_time += 
                        (selection_stop.tv_sec - selection_start.tv_sec) + 
                        (selection_stop.tv_nsec - selection_start.tv_nsec)/1e9;
                /* END SELECTION */

                /* CROSSOVER */
                clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &crossover_start);

                parameterized_uniform_crossover(archipelago[isl],
                                                parent_idxs,
                                                graph->v,
                                                &(children[idv]),
                                                &(children[idv+1])
                                               );

                clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &crossover_stop);
                crossover_time += 
                        (crossover_stop.tv_sec - crossover_start.tv_sec) +
                        (crossover_stop.tv_nsec - crossover_start.tv_nsec)/1e9;
                /* END CROSSOVER */

                /* MUTATION */
                clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &mutation_start);

                for (int childno=0; childno<2; childno++) {
                    for (int locus=0; locus<graph->v; locus++) {

                        double mutation_decision1 = (double)rand()/RAND_MAX;
                        if (mutation_decision1 < MUTATION_PROB) {
                            // mutate: 1->0 or 0->1
                            putbit(children[idv+childno].partition,
                                   locus,
                                   !getbit(children[idv+childno].partition, locus)
                                  );
                        }
                    }
                }

                clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &mutation_stop);
                mutation_time += 
                        (mutation_stop.tv_sec - mutation_start.tv_sec) +
                        (mutation_stop.tv_nsec - mutation_start.tv_nsec)/1e9;
                /* END MUTATION */

                // CALCULATE FITNESS OF NEW CHILDREN
                // TODO: move this outside of the loop so that the hardware can do
                // them all in parallel
                clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &fitness_start);

                children[idv].fitness = calc_fitness(graph, &(children[idv]));
                children[idv+1].fitness = calc_fitness(graph, &(children[idv+1]));

                clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &fitness_stop);
                fitness_time += 
                        (fitness_stop.tv_sec - fitness_start.tv_sec) + 
                        (fitness_stop.tv_nsec - fitness_start.tv_nsec)/1e9;

            } /* END GENETIC OPERATORS (SELECTION, CROSSOVER, MUTATION) */

            // Replace the island's current population with the new children
            for (int idv=0; idv<POP_SIZE; idv++) {
                for (int j=0; j<RESERVE_BITS(graph->v); j++) {
                    archipelago[isl][idv].partition[j] 
                            = children[idv].partition[j];
                }
                archipelago[isl][idv].fitness = children[idv].fitness;
                //total_inverse_fitness += 1.0/(double)archipelago[0][i].fitness;
            }

            // free the children
            for (int idv=0; idv<POP_SIZE; idv++) {
               free(children[idv].partition);
            }
            free(children);

        } /* END ISLAND LOOP */

    } /* END EVOLUTIONARY LOOP */

    printf("\r%d generations complete.  \n", NUM_GENERATIONS);

    // print best individual
    int min_fitness = INT_MAX;
    int min_isl_idx = -1;
    int min_idv_idx = -1;
    for (int isl=0; isl<NUM_ISLANDS; isl++) {
        for (int idv=0; idv<POP_SIZE; idv++) {
            if (archipelago[isl][idv].fitness < min_fitness) {
                min_fitness = archipelago[isl][idv].fitness;
                min_isl_idx = isl;
                min_idv_idx = idv;
            }
       }
    }

    printf("Most fit individual was found on island %d: ", min_isl_idx);
    int p0_cnt = 0;
    int p1_cnt = 0;
    for (int i=0; i<graph->v; i++) {
        if (getbit(archipelago[min_isl_idx][min_idv_idx].partition, i) == 0)
            p0_cnt++;
        else
            p1_cnt++;

        //printf("%d", getbit(population[min_idx].partition, i));        
    }
    printf("\n");
    int external_cost = 0;
    for (int i=0; i<graph->e; i++) {
        if (getbit(archipelago[min_isl_idx][min_idv_idx].partition, 
                   (graph->edges)[i]->n1) 
            != getbit(archipelago[min_isl_idx][min_idv_idx].partition, 
                      (graph->edges)[i]->n2)) {
            
            external_cost += (graph->edges)[i]->weight;
        }
    }
    printf("\tFitness = %d\n", archipelago[min_isl_idx][min_idv_idx].fitness);
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
    printf("\tTime spent in migration: %8.2f sec (%4.1f%%)\n",
           migration_time,
           (migration_time/total_time)*100
          );

    // free islands
    /*
    for (int i=0; i<NUM_ISLANDS; i++) {
        traverseList(archipelago[i].member_list, &free);
        removeAllNodes(archipelago[i].member_list);
        free(archipelago[i].member_list);
        free(archipelago[i].migration_probs);
    }
    free(archipelago);*/

    // Free memory used for migration
    for (int isl=0; isl<NUM_ISLANDS; isl++) {
        free(sorted_indices_list[isl]);
    }
    free(sorted_indices_list);

    // Free population on each island
    for (int isl=0; isl<NUM_ISLANDS; isl++) {
        for (int idv=0; idv<POP_SIZE; idv++) {
            free(archipelago[isl][idv].partition);
        }
        free(archipelago[isl]);
    }

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
 * Initializes random partitions for POP_SIZE individuals in a given 
 * population (array of individuals)
 */
void init_population(Individual* population, int num_nodes) {

    for (int i=0; i<POP_SIZE; i++) {
        // allocate memory for the individual's partition
        population[i].partition =
                malloc(RESERVE_BITS(num_nodes) * sizeof(bitarray_t));
        CHECK_MALLOC_ERR(population[i].partition);

        // initialize partitions to 0:
        for (int j=0; j<RESERVE_BITS(num_nodes); j++) {
            (population[i].partition)[j] = 0;
        }

        // create a random starting partition:
        for (int j=0; j<num_nodes; j++) {
            int rand_bit = urandint(2);
            putbit(population[i].partition, j, rand_bit);
        }
    }
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

// FUNCTIONS FOR INTERACTING WITH GAA_FITNESS DEVICE DRIVER:

/*
 * Read and print output value
 */
uint8_t print_output_value(const int fd) {

    gaa_fitness_arg_t gaa_arg;

    if (ioctl(fd, GAA_FITNESS_READ_OUTPUTS, &gaa_arg)) {
        perror("ioctl(GAA_FITNESS_READ_OUTPUTS) failed");
        return 0;
    }

    printf("0x%X xor 0x%X = 0x%X\n",
	       gaa_arg.inputs.p1, gaa_arg.inputs.p2, gaa_arg.outputs.p1xorp2);

    return gaa_arg.outputs.p1xorp2;
}

/*
 * Set the inputs
 */
void set_input_values(const gaa_fitness_inputs_t* i, const int fd) {

    gaa_fitness_arg_t gaa_arg;
    gaa_arg.inputs = *i;

    if (ioctl(fd, GAA_FITNESS_WRITE_INPUTS, &gaa_arg)) {
        perror("ioctl(GAA_FITNESS_WRITE_INPUTS) failed");
        return;
    }
}

