/*
 * GAA.h
 *
 * Header file for GAA.c
 *
 */

#ifndef _GAA_H_
#define _GAA_H_

#include "ga-params.h"
#include "graph.h"

double calc_diversity (Individual*, int);
int    calc_fitness   (Graph*, Individual*);
void   init_population(Individual*, int);
void   shuffle        (int*, int);

#endif  /* _GAA_H */

