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

#include "GAA.h"

#define POP_SIZE 10

int calc_fitess(Graph*, Individual*);

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



    // initialize population
    Individual* population = malloc(POP_SIZE * sizeof(Individual));
    CHECK_MALLOC_ERR(population);
    for (int i=0; i<POP_SIZE; i++) {
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

        // initialize fitness to 0;
        //population[i].fitness = 0;

        // calculate fitness:
        population[i].fitness = calc_fitness(graph, &(population[i]));

        printf("Individual %d:\n", i);
        printf("\tpartition: ");
        for( int j=0; j<graph->v; j++) {
            printf("%d", getbit(population[i].partition, j));
        }
        printf("\n");
        printf("\tfitness = %d\n", population[i].fitness);
    }







    // evolution loop
    for (int gen=0; gen<NUM_OF_GENERATIONS; gen++) {
        /* crossover */
        // initialize child population
        Individual* children = malloc(POP_SIZE * sizeof(Individual));
        CHECK_MALLOC_ERR(children);
        for (int i=0; i<POP_SIZE; i++) {
            children[i].partition = malloc(RESERVE_BITS(graph->v) * sizeof(bitarray_t));
            CHECK_MALLOC_ERR(children[i].partition);
            // initialize partitions to values in parent population
            for (int j = 0; j < RESERVE_BITS(graph->v); j++) {
                (children[i].partition)[j] = (population[i].partition)[j];
            }
            // create random order to crossover
            int order[POP_SIZE];
            shuffle(order, POP_SIZE);
            // crossover, currently single point crossover, change to at least two point later
            for (int i = 0; i < POP_SIZE; i += 2) {
                int crossover_point = rand() % graph->v; // random point
                bitarray_t temp[graph->v];
                for (int j = crossover_point; j < graph->v; j++) {
                    temp[j] = children[i].partition[j];
                    children[i].partition[j] = children[i + 1].partition[j];
                    children[i + 1].partition[j] = temp[j];
                }
            }
        }

        // free the children
        for (int i=0; i<POP_SIZE; i++) {
            free(children[i].partition);
        }
        free(children);
    } // end of evolution loop



    // free population
    for (int i=0; i<POP_SIZE; i++) {
        free(population[i].partition);
    }
    free(population);

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

int calc_fitness(Graph* graph, Individual* indiv) {
    return 0;
}

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

