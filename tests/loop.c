#include <loki/lokilib.h>
#include <stdio.h>
#include <stdlib.h>

#define ITERATIONS 1000

// This program computes the sum of integers in the range 0-1000 using a
// parallel loop.

// Each core needs to keep track of its own sum. These can all be combined when
// all cores have finished.
int* partial_sum;

// A single iteration of the SIMD loop.
void iteration(int iteration, int coreid) {
  partial_sum[coreid] += iteration;
}

// Combine all of the partial results into a single result. In many cases this
// function will be left empty.
void reduce(int num_cores) {
  int sum = 0;

  int i;
  for (i=0; i<num_cores; i++)
    sum += partial_sum[i];

  if (sum != ((ITERATIONS - 1) * ITERATIONS) / 2) {
    fprintf(stderr, "Incorrect sum(1 to %d) of %d not %d.\n", ITERATIONS - 1, sum, ((ITERATIONS - 1) * ITERATIONS) / 2);
    exit(EXIT_FAILURE);
  }
}

int main (int argc, char** argv) {

  // For the moment, this function depends on the cache configuration being in
  // a register, so it must be the first thing executed. The cache configuration
  // needs to be shared with the other cores, so they can all access memory in
  // the same way.
  init_cores(8);

  // Prepare all information needed to execute the loop.
  loop_config loop;
  loop.cores = 8;
  loop.iterations = ITERATIONS;
  loop.initialise = 0;
  loop.helper_init = 0;
  loop.iteration = &iteration;
  loop.helper = 0;
  loop.tidy = 0;
  loop.reduce = &reduce;


  // Execute parallel loop.

  // A SIMD loop can have 1-8 cores, inclusive.
  partial_sum = calloc(loop.cores, sizeof(int));
  simd_loop(&loop);
  free(partial_sum);

  // A worker farm can have 2-6 cores, inclusive (need at least 1 master and 1
  // worker, and the number of workers is currently limited by the number of
  // connections which can simultaneously be made to the master core).
  loop.cores = 6;
  partial_sum = calloc(loop.cores, sizeof(int));
  worker_farm(&loop);
  free(partial_sum);

  exit(EXIT_SUCCESS);
}
