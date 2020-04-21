/*
 * graph-parser.c
 *
 * Take in file name and path and fill in a Graph object with the data
 *
 */

#include <assert.h>  // assert
#include <stdio.h>   // printf, fgets
#include <stdlib.h>  // atoi, malloc
#include <string.h>  // strrchr, strcmp, strtok

#include "ga-utils.h"
#include "graph-parser.h"

/*
 * Parses a graph struct from a file. Returns 1 on success, 0 on failure
 */
int parse_graph_from_file(char* filename, Graph* graph) {

    char* filetype;
    int format;
    FILE* fp;
    char line[1024];
    int num_nodes, num_edges;
    char* separators = "\t \n";

    // check to make sure input file is a compatable file type
    filetype = strrchr(filename, '.');
    if (filetype && strcmp(filetype, ".edgelist") == 0) {
        format = EL;
    }
    else {
        fprintf(stderr, "%s\n", "supported graph file formats are: .el");
        return 0;
    }

    // open input file specified
    fp = fopen(filename, "r");
    if (NULL == fp) {
        perror(filename);
        return 0;
    }

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
                return 0;
            }

            if (fgets(line, sizeof(line), fp)) {
                strtok(line, separators);
                num_edges = atoi(strtok(NULL, separators));
                graph->e = num_edges;
                printf("Number of edges: %d\n", num_edges);
            }
            else {
                fprintf(stderr, "Invalid file!\n");
                return 0;
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

    return 1;
}

