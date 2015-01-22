#include <loki/lokilib.h>
#include <stdio.h>
#include <stdlib.h>

#define END_OF_STREAM (-1)

// A very simple pipeline with three stages, used to compute x^2 + 1.
// This version is data-driven: the stages pass data directly between each other,
// instead of requiring that it be stored in an intermediate buffer.

// The first stage is always responsible for supplying the data. It therefore
// needs to know when to stop. It's input is always the iteration.
int stage1(int iteration) {
  if (iteration < 10)
    return iteration * iteration;
  else
    return END_OF_STREAM;
}

// All subsequent stages receive a single argument (of any type) from the
// previous stage, and implicitly send their return value (of any type) to the
// next stage.
int stage2(int data) {
  return data + 1;
}

// Note that this stage is much slower than the other stages. There must be some
// form of flow control to stop the previous stages producing results faster
// than they can be handled here.
int stage3(int data) {
  static int iteration = 0;
  if (data != iteration * iteration + 1) {
    fprintf(stderr, "Incorrect result at iteration %d. %d != %d.\n", iteration, data, iteration * iteration + 1);
    exit(EXIT_FAILURE);
  }
  iteration++;
  return 0; // Return value here is ignored.
}

int main(int argc, char** argv) {

  dd_pipeline_config pipeline;
  pipeline.cores = 3;
  pipeline.end_of_stream_token = END_OF_STREAM;

  loki_init_default(pipeline.cores, 0);

  pipeline.stage_tasks = malloc(pipeline.cores * sizeof(dd_pipeline_func));
  pipeline.stage_tasks[0] = &stage1;
  pipeline.stage_tasks[1] = &stage2;
  pipeline.stage_tasks[2] = &stage3;

  dd_pipeline_loop(&pipeline);

  exit(EXIT_SUCCESS);
}
