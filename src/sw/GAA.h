/*
 * GAA.h
 *
 * Header file for GAA.c
 *
 */

#ifndef _GAA_H_
#define _GAA_H_

#include <stdint.h>

#include "ga-params.h"
#include "gaa_fitness_driver.h"
#include "graph.h"

double calc_diversity (Individual*, int);
int    calc_fitness   (Graph*, Individual*);
void   init_population(Individual*, int);
void   shuffle        (int*, int);

// funcitons for interacting with device driver
uint8_t print_output_value(const int);
void    set_input_values  (const gaa_fitness_inputs_t*, const int); 

#endif  /* _GAA_H */

