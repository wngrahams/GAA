/*
 * hello.c
 *
 * test program to verify functionality of gaa_fitness device driver
 */

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "gaa_fitness.h"


int gaa_fitness_fd;

/* Read and print the output value */
uint8_t print_output_value() {
  gaa_fitness_arg_t gaa_arg;

  if (ioctl(gaa_fitness_fd, GAA_FITNESS_READ_OUTPUTS, &gaa_arg)) {
      perror("ioctl(GAA_FITNESS_READ_OUTPUTS) failed");
      return 0;
  }
  printf("0x%X xor 0x%X = 0x%X\n",
	 gaa_arg.inputs.p1, gaa_arg.inputs.p2, gaa_arg.outputs.p1xorp2);

  return gaa_arg.outputs.p1xorp2;
}

/* Set the inputs */
void set_input_values(const gaa_fitness_inputs_t *i)
{
  gaa_fitness_arg_t gaa_arg;
  gaa_arg.inputs = *i;
  if (ioctl(gaa_fitness_fd, GAA_FITNESS_WRITE_INPUTS, &gaa_arg)) {
      perror("ioctl(GAA_FITNESS_WRITE_INPUTS) failed");
      return;
  }
}

int main()
{
  gaa_fitness_arg_t gaa_arg;
  uint8_t i, j, out;
  static const char filename[] = "/dev/gaa_fitness";
  
  static const gaa_fitness_inputs_t initial_inputs = { 0x0F , 0xF0 };

  gaa_fitness_inputs_t current_inputs = {.p1 = initial_inputs.p1, 
                                         .p2 = initial_inputs.p2};

  printf("GAA Fitness Userspace program started\n");

  if ( (gaa_fitness_fd = open(filename, O_RDWR)) == -1) {
    fprintf(stderr, "could not open %s\n", filename);
    return -1;
  }

  set_input_values(&initial_inputs);
  out = print_output_value();
  //assert(out == 0xFF);

  for (i=0x00, j=0xF0; i<=0xF0 && j>=0x00; i++, j--) {
    current_inputs.p1 = i;
    current_inputs.p2 = j;
    set_input_values(&current_inputs);
    out = print_output_value();
    //usleep(500000);
  }

  printf("GAA Fitness Userspace program terminating\n");
  return 0;
}

