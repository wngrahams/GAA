/*
 * graph-parser.h
 *
 * header file for graph-parser.c
 *
 */

#ifndef _GRAPH_PARSER_H_
#define _GRAPH_PARSER_H_

#include "graph.h"

// filetype macros
#define EL    0
#define WEL   1
#define DOT   2
#define GZ    3
#define GRAPH 4

int parse_graph_from_file(char*, Graph*);

#endif  /* _GRAPH_PARSER_ */
