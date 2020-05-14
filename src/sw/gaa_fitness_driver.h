/*
 * gaa_fitness_driver.h
 *
 * Header files that defines ioctls for reading and writing with gaa_fitness
 * peripheral
 */

#ifndef _GAA_FITNESS_DRIVER_H_
#define _GAA_FITNESS_DRIVER_H_

#include <linux/ioctl.h>
#include <linux/types.h>

typedef struct {
    uint16_t p1, p2;  // u8 is an unsigned byte
} gaa_fitness_inputs_t;

typedef struct {
    uint16_t p1xorp2;
} gaa_fitness_outputs_t;

typedef struct {
    gaa_fitness_inputs_t inputs;
    gaa_fitness_outputs_t outputs;
} gaa_fitness_arg_t;

#define GAA_FITNESS_MAGIC 'q'

/* ioctls and their arguments */
#define GAA_FITNESS_WRITE_INPUTS _IOW(GAA_FITNESS_MAGIC, 1, gaa_fitness_arg_t*)
#define GAA_FITNESS_READ_OUTPUTS _IOR(GAA_FITNESS_MAGIC, 2, gaa_fitness_arg_t*)

#endif /* _GAA_FITNESS_DRIVER_H_ */

