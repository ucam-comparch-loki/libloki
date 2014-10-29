#include "../lokilib.h"
#include <stdio.h>
#include <stdlib.h>

// A very simple pipeline with three stages, used to compute x^2 + 1.
// There must currently be a buffer between each pair of pipeline stages - I
// hope to eventually pass any data directly between the stages, eliminating the
// need for this.

int* stage_1_2_buffer;
int* stage_2_3_buffer;

void stage1(int iteration) {
  stage_1_2_buffer[iteration] = iteration*iteration;
}

void stage2(int iteration) {
  stage_2_3_buffer[iteration] = stage_1_2_buffer[iteration] + 1;
}

// Note that this stage is much slower than the other stages. There must be some
// form of flow control to stop the previous stages producing results faster
// than they can be handled here.
void stage3(int iteration) {
  printf("%d -> %d\n", iteration, stage_2_3_buffer[iteration]);
}

int main(int argc, char** argv) {
  
  pipeline_config pipeline;
  pipeline.cores = 3;
  pipeline.iterations = 10;
  
  init_cores(pipeline.cores);
  
  pipeline.stage_func = malloc(pipeline.cores * sizeof(pipeline_func));
  pipeline.stage_func[0] = &stage1;
  pipeline.stage_func[1] = &stage2;
  pipeline.stage_func[2] = &stage3;
  
  stage_1_2_buffer = malloc(pipeline.iterations * sizeof(int));
  stage_2_3_buffer = malloc(pipeline.iterations * sizeof(int));
  
  pipeline_loop(&pipeline);
  
  return 0;
  
}
