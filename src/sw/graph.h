/*
 * graph.h
 *
 * Defines the data structure 'Graph' used in this project, along with the 
 * data structures for the nodes and edges of the graph
 *
 */

#ifndef _GRAPH_H_
#define _GRAPH_H_

typedef struct Node {
    int id;      // node id
    int weight;  // node weight
} Node;

typedef struct Edge {
    int n1;      // id of node 1
    int n2;      // id of node 2
    int weight;  // edge weight
} Edge;

typedef struct Graph {
    int v;         // number of nodes
    int e;         // number of edges
    Node** nodes;  // array of pointers to nodes
    Edge** edges;  // array of pointers to edges
} Graph;

#endif /* _GRAPH_H_ */

