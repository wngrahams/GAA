/*
 * GAA.c
 *
 * Graph partitioning using a genetic algorithm with hardware acceleration
 *
 */

#include <assert.h>  // assert
#include <stdio.h>   // printf, fgets
#include <stdlib.h>  // atoi
#include <string.h>  // strrchr, strcmp, strtok
#include <time.h>   // time

#include "GAA.h"

#define POP_SIZE 10

int calc_fitness(Graph*, Individual*);

int main(int argc, char** argv) {

    int format;
    char *filename, *filetype;
    FILE* fp;
    char line[1024];
    char* separators = "\t \n";
    int num_nodes, num_edges;

    Graph* graph;
 
    if (argc != 2) {
        fprintf(stderr, "%s\n", "usage: gaa <graph_file>");
        exit(1);
    }

    filename = argv[1];

    // check to make sure input file is a compatable file type
    filetype = strrchr(filename, '.');
    if (filetype && strcmp(filetype, ".edgelist") == 0) {
        format = EL;
    }
    else {
        fprintf(stderr, "%s\n", "supported graph file formats are: .el");
        exit(1);
    }

    // open input file specified in command line
    fp = fopen(filename, "r");
    if (NULL == fp) {
        perror(filename);
        exit(1);
    }

    // allocate memory for a graph struct
    graph = malloc(sizeof(Graph));
    CHECK_MALLOC_ERR(graph);

    switch (format) {
        case EL:
            // read the first two lines to get the number of nodes and edges
            if (fgets(line, sizeof(line), fp)) {
                strtok(line, separators);
                num_nodes = atoi(strtok(NULL, separators));
                graph->v = num_nodes;
                printf("Number of nodes: %d\n", num_nodes);
            }
            else {
                fprintf(stderr, "Invalid file!\n");
                exit(1);
            }

            if (fgets(line, sizeof(line), fp)) {
                strtok(line, separators);
                num_edges = atoi(strtok(NULL, separators));
                graph->e = num_edges;
                printf("Number of edges: %d\n", num_edges);
            }
            else {
                fprintf(stderr, "Invalid file!\n");
                exit(1);
            }

            // allocate memory for the graphs node and edge lists:
            graph->nodes = malloc(num_nodes * sizeof(Node*));
            CHECK_MALLOC_ERR(graph->nodes);
            graph->edges = malloc(num_edges * sizeof(Edge*));
            CHECK_MALLOC_ERR(graph->edges);

            int edge_cnt = 0;
            int node_cnt = 0;
            int left_num, right_num;

            // read the rest of the file, adding nodes and edges
            // this assumes that the nodes in the file are in ascending order,
            // i.e. the first apperance of node 3 will not be before the first
            // apperance of node 2
            while (fgets(line, sizeof(line), fp)) {

                left_num = atoi(strtok(line, separators));
                right_num = atoi(strtok(NULL, separators));

                if (left_num == node_cnt) {

                    assert(node_cnt < num_nodes);

                    // a new node we haven't seen before:
                    Node* new_node = malloc(sizeof(Node));
                    CHECK_MALLOC_ERR(new_node);
                    new_node->id = left_num;
                    new_node->weight = 1;

                    // add new node to graph
                    (graph->nodes)[node_cnt] = new_node;

                    node_cnt++;
                }

                assert(node_cnt <= num_nodes);

                if (right_num == node_cnt) {
                    
                    assert(node_cnt < num_nodes);

                    // a new node we haven't seen before:
                    Node* new_node = malloc(sizeof(Node));
                    CHECK_MALLOC_ERR(new_node);
                    new_node->id = right_num;
                    new_node->weight = 1;

                    // add new node to graph
                    (graph->nodes)[node_cnt] = new_node;

                    node_cnt++;
                }
                
                // new edge:
                assert(edge_cnt < num_edges);

                Edge* new_edge = malloc(sizeof(Edge));
                CHECK_MALLOC_ERR(new_edge);

                new_edge->n1 = left_num;
                new_edge->n2 = right_num;
                new_edge->weight = 1;

                // add new edge to graph
                (graph->edges)[edge_cnt] = new_edge;

                edge_cnt++;

            } /* END while fgets */
            
            assert(graph->v == node_cnt);
            assert(graph->e == edge_cnt);

            break;  /* END case(EL) */ 

        default:
            ;
    }  /* END switch */

    // close file
    fclose(fp);

    
    // print graph
    /*
    printf("Graph:\n");
    printf("\t|v| = %d\n", graph->v);
    printf("\t|e| = %d\n", graph->e);
    printf("Nodes:\n");
    for (int i=0; i<graph->v; i++) {
        printf("\tid: %d, weight: %d\n", 
                        (graph->nodes)[i]->id, (graph->nodes)[i]->weight);
    }
    printf("Edges:\n");
    for (int i=0; i<graph->e; i++) {
        printf("\tn1: %d, n2: %d, weight: %d\n", 
                        (graph->edges)[i]->n1, (graph->edges)[i]->n2,
                        (graph->edges)[i]->weight);
    }*/


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
        goto cleanup_graph;
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
        

        printf("Individual %d:\n", i);
        printf("\tpartition: ");
        for( int j=0; j<graph->v; j++) {
            printf("%d", getbit(population[i].partition, j));
        }
        printf("\n");
        printf("\tfitness = %d\n", population[i].fitness);

    } /* END initialize population */

    // TODO: calculate initial fitnesses in hardware here
    //  ---------

    printf("Total inverse fitness: %f\n", total_inverse_fitness);
    printf("RAND_MAX: %d\n", RAND_MAX);

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
                                            * rand()/RAND_MAX;
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

            printf("selected parents %d and %d.\n",parent_idxs[0],parent_idxs[1]);

            // CROSSOVER:
            // With probability CROSSOVER_PROB (the "crossover probability" or 
            // "crossover rate"), cross over the pair at a randomly chosen 
            // point (chosen with uniform probability) (TODO) to form two 
            // offspring. If no crossover takes place, form two offspring that 
            // are exact copies of their respective parents.

            double crossover_decision = rand()/RAND_MAX;
            if (crossover_decision < CROSSOVER_PROB) {

                printf("Performing crossover... ");

                // single point crossover -- TODO: change to multiple point, 
                //     where crossover rate for a pair of parents is the number
                //     of points at which a crossover takes place.

                // choose the bit at which to crossover: (TODO better random)
                // doesn't pick bit 0 because that is the same as no crossover
                int crossover_point = (rand()%(graph->v-1)) + 1;  // (0, graph->v)
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

                printf("\tChildren: ");
                for( int j=0; j<graph->v; j++) {
                    printf("%d", getbit(children[i].partition, j));
                }
                printf(", ");
                for( int j=0; j<graph->v; j++) {
                    printf("%d", getbit(children[i+1].partition, j));
                }
                printf("\n");

            }
            else {

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
            for (int childno=0; childno<2; childno++) {
                for (int locus=0; locus<graph->v; locus++) {
                    
                    double mutation_decision1 = rand()/RAND_MAX;
                    if (mutation_decision1 < MUTATION_PROB) {
                        
                        // mutate: 1->0 or 0->1
                        putbit(children[i+childno].partition,
                               locus,
                               !getbit(children[i+childno].partition, locus)
                              );
                    }

                }
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


cleanup_all:
    // free population
    for (int i=0; i<POP_SIZE; i++) {
        free(population[i].partition);
    }
    free(population);

cleanup_graph:
    // free memory used for graph:
    for (int i=0; i<graph->v; i++) {
        free((graph->nodes)[i]);
    }
    free(graph->nodes);
    for (int i=0; i<graph->e; i++) {
        free((graph->edges)[i]);
    }
    free(graph->edges);
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

