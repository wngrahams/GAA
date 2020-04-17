/*
 * GAA.c
 *
 * Graph partitioning using a genetic algorithm with hardware acceleration
 *
 */

#include <stdio.h>   // printf, fgets
#include <stdlib.h>  // atoi
#include <string.h>  // strrchr, strcmp, strtok

#define EL 0


int main(int argc, char** argv) {

    int format;
    char *filename, *filetype;
    FILE* fp;
    char line[1024];
    char* separators = "\t \n";
    int num_nodes, num_edges;
    char *left, *right;
 
    if (argc != 2) {
        fprintf(stderr, "%s\n", "usage: gaa <graph_file>");
        exit(1);
    }

    filename = argv[1];

    // check to make sure input file is a compatable file type
    filetype = strrchr(filename, '.');
    if (filetype && strcmp(filetype, ".el") == 0) {
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

    switch (format) {
        case EL:
            // read the first two lines to get the number of nodes and edges
            if (fgets(line, sizeof(line), fp)) {
                left = strtok(line, separators);
                right = strtok(NULL, separators);
                num_nodes = atoi(right);
                printf("Number of nodes: %d\n", num_nodes);
            }
            else {
                fprintf(stderr, "Invalid file!\n");
                exit(1);
            }

            if (fgets(line, sizeof(line), fp)) {
                left = strtok(line, separators);
                right = strtok(NULL, separators);
                num_edges = atoi(right);
                printf("Number of edges: %d\n", num_edges);
            }
            else {
                fprintf(stderr, "Invalid file!\n");
                exit(1);
            }




        default:
            ;
    }

    return 0;
}

