#include "../lokilib.h"
#include <stdio.h>
#include <stdlib.h>

#define END_OF_STREAM (-1)

// A very simple pipeline with three stages, used to compute x^2 + 1.
// This version is data-driven: the stages pass data directly between each other,
// instead of requiring that it be stored in an intermediate buffer.

// The first stage is always responsible for supplying the data. It therefore
// needs its own loop so it knows when to stop.
void stage1() {
  int i;
  for (i=0; i<10; i++)
    SEND(i*i, 2);   // The next pipeline stage is connected to output channel 2.
    
  SEND(END_OF_STREAM, 2);
}

// All subsequent stages receive a single argument (of any type) from the
// previous stage, and implicitly send their return value (of any type) to the
// next stage.
int stage2(int data) {
  if (data == END_OF_STREAM)
    return data;
  else
    return data + 1;
}

// Note that this stage is much slower than the other stages. There must be some
// form of flow control to stop the previous stages producing results faster
// than they can be handled here.
void stage3(int data) {
  static int iteration = 0;
  
  if (data == END_OF_STREAM)
    end_parallel_section();
  else
    printf("%d -> %d\n", iteration++, data);
}

int main(int argc, char** argv) {
  
  dd_pipeline_config pipeline;
  pipeline.cores = 3;
  
  init_cores(pipeline.cores);
  
  pipeline.stage_tasks = malloc(pipeline.cores * sizeof(dd_pipeline_func));
  pipeline.stage_tasks[0] = &stage1;
  pipeline.stage_tasks[1] = &stage2;
  pipeline.stage_tasks[2] = &stage3;
  
  dd_pipeline_loop(&pipeline);
  
  return 0;
  
}
