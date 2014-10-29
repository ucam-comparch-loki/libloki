#include "../lokilib.h"
#include <stdio.h>
#include <stdlib.h>

// This program computes the sum of integers in the range 0-1000 using a
// parallel loop. The number of cores can be specified on the command line.

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
    
  printf ("sum = %d\n", sum);
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
  loop.iterations = 1000;
  loop.iteration = &iteration;
  loop.reduce = &reduce;
  
  // Allow number of cores to be specified on command line.
  if (argc > 1)
    loop.cores = atoi(argv[1]);
    
  partial_sum = malloc(loop.cores * sizeof(int));
  
  // Execute parallel loop. 
  
  // A SIMD loop can have 1-8 cores, inclusive.
  simd_loop(&loop);
  
  // A worker farm can have 2-6 cores, inclusive (need at least 1 master and 1 
  // worker, and the number of workers is currently limited by the number of 
  // connections which can simultaneously be made to the master core).
//  worker_farm(&loop);

}
