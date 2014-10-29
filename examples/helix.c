#include "../lokilib.h"
#include <stdio.h>
#include <stdlib.h>

// This program computes the sum of integers in the range 0-1000 using a
// parallel loop. The number of cores can be specified on the command line.
// For this example, the loop has effectively been sequentialised by creating
// a dependency between each iteration.
int sum;

// Any work to be done before the loop begins. In this case, we set the
// appropriate channel map table entries and send initial values.
void initialise(int cores, int iterations, int coreid) {
  // Connect all cores in a helix, where each core is connected to the core 3
  // around the ring. Send on output 2, receive on input 3.
  HELIX_CONNECT(2 /*output*/, 3 /*next core*/, 3 /*input*/, cores);
  
  // Simulate an "iteration -1" so that iteration 0 gets the appropriate live-in
  // value(s). In this case, this just means sending 0 (sum so far) to core 0.
  if (coreid == cores-3)
    SEND(0,2);
}

// A single iteration of the SIMD loop.
void iteration(int iteration, int coreid) {
  int sum_so_far;
  RECEIVE(sum_so_far, 3);
  sum = sum_so_far + iteration;
  SEND(sum, 2);
}

// Since every iteration of the loop results in some data being sent (including
// the final one), there will be one extra value floating around after the loop 
// has finished.
void tidy(int cores, int iterations, int coreid) {
  int core_with_extra_val = (3*(iterations)) % cores;
  if (coreid == core_with_extra_val) {
    int dummy;
    RECEIVE(dummy, 3);
  }
}

// Combine all of the partial results into a single result. In many cases this
// function will be left empty.
void reduce(int num_cores) {
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
  loop.initialise = &initialise;
  loop.iteration = &iteration;
  loop.tidy = &tidy;
  loop.reduce = &reduce;
  
  // Allow number of cores to be specified on command line.
  if (argc > 1)
    loop.cores = atoi(argv[1]);
  
  // Execute parallel loop.  
  // A SIMD loop can have 1-8 cores, inclusive.
  simd_loop(&loop);

}
