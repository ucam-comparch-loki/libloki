// Complex number multiplication.
// (a + bi) * (c + di) = (ac - bd) + (bc + ad)i
// Perform each multiplication/addition/subtraction on a separate core.

//                                Core 0
//                            collate inputs
//                       ____/   /       \   \____
//                      /       /         \       \
//                Core 1     Core 2     Core 4     Core 5
//                 a*c        b*d         b*c        a*d
//                     \     /               \      /
//                     Core 3                 Core 6
//                    ac - bd                 bc + ad
//                           \____       ____/
//                                \     /
//                                Core 7
//                             store outputs

#include <loki/lokilib.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_INPUTS 5

int *real_in,  *imag_in;
int *real_out, *imag_out;

// Distribute operands - this is going to be a huge bottleneck, but I don't care
// for this test.
// An alternative mapping would be to have core0 handle real inputs and outputs,
// and core 7 handle imaginary inputs and outputs.
void core0() {
  // Connect to cores 1, 2, 4 and 5 (two connections each, for each operand)
  // CONNECT(output -> (tile, component, channel))
  CONNECT(2,   0,1,3);
  CONNECT(3,   0,1,4);
  CONNECT(4,   0,2,3);
  CONNECT(5,   0,2,4);
  CONNECT(6,   0,4,3);
  CONNECT(7,   0,4,4);
  CONNECT(8,   0,5,3);
  CONNECT(9,   0,5,4);

  // Distribute operands to cores.
  int i, j;
  for (i=0; i<NUM_INPUTS; i++) {
    for (j=0; j<NUM_INPUTS; j++) {
      const int a = real_in[i], b = imag_in[i];
      const int c = real_in[j], d = imag_in[j];

      // Multicast would help a little here.
      SEND(a, 2); SEND(a, 8);
      SEND(b, 4); SEND(b, 6);
      SEND(c, 3); SEND(c, 7);
      SEND(d, 5); SEND(d, 9);
    }
  }

}

void multicast_core0() {
  int bitmask, address;
  // send a to cores 1 and 5, input 3
  bitmask = 0x22;
  address = loki_mcast_address(0, bitmask, 3);
  SET_CHANNEL_MAP(2, address);
  // send b to cores 2 and 4, input 3
  bitmask = 0x14;
  address = loki_mcast_address(0, bitmask, 3);
  SET_CHANNEL_MAP(3, address);
  // send c to cores 1 and 4, input 4
  bitmask = 0x12;
  address = loki_mcast_address(0, bitmask, 4);
  SET_CHANNEL_MAP(4, address);
  // send d to cores 2 and 5, input 4
  bitmask = 0x24;
  address = loki_mcast_address(0, bitmask, 4);
  SET_CHANNEL_MAP(5, address);
  // vulnerable to missing eop bug!
  asm volatile ("fetchr.eop 0f\n0:\n");

  // Distribute operands to cores.
  int i, j;
  for (i=0; i<NUM_INPUTS; i++) {
    for (j=0; j<NUM_INPUTS; j++) {
      const int a = real_in[i], b = imag_in[i];
      const int c = real_in[j], d = imag_in[j];

      SEND(a, 2);
      SEND(b, 3);
      SEND(c, 4);
      SEND(d, 5);
    }
  }

}

// a * c
void core1() {
  // Connect to core 3
  CONNECT(2,   0,3,3);

  DATAFLOW_PACKET (1,
    "mullw.eop r0, r3, r4 -> 2\n"
  );
}

// b * d
void core2() {
  // Connect to core 3
  CONNECT(2,   0,3,4);

  DATAFLOW_PACKET (2,
    "mullw.eop r0, r3, r4 -> 2\n"
  );
}

// ac - bd
void core3() {
  // Connect to core 7
  CONNECT(2,   0,7,3);

  DATAFLOW_PACKET (3,
    "subu.eop r0, r3, r4 -> 2\n"
  );
}

// b * c
void core4() {
  // Connect to core 6
  CONNECT(2,   0,6,3);

  DATAFLOW_PACKET (4,
    "mullw.eop r0, r3, r4 -> 2\n"
  );
}

// a * d
void core5() {
  // Connect to core 6
  CONNECT(2,   0,6,4);

  DATAFLOW_PACKET (5,
    "mullw.eop r0, r3, r4 -> 2\n"
  );
}

// bc + ad
void core6() {
  // Connect to core 7
  CONNECT(2,   0,7,4);

  DATAFLOW_PACKET (6,
    "addu.eop r0, r3, r4 -> 2\n"
  );
}

// Store results
void core7() {

  // Receive results
  volatile int i, j;
  for (i=0; i<NUM_INPUTS; i++) {
    for (j=0; j<NUM_INPUTS; j++) {
      int real, imag;
      RECEIVE(real, 3);
      RECEIVE(imag, 4);

      real_out[i*NUM_INPUTS + j] = real;
      imag_out[i*NUM_INPUTS + j] = imag;
    }
  }

  // Signal that all results have now been produced.
  end_parallel_section();

  // All cores are supposed to use DATAFLOW_PACKET macro, which has the behaviour of a loop. Sleeping should suffice.
  loki_sleep();
}

int main(int argc, char** argv) {

  init_cores(8);

  // Initialise input/output arrays.
  real_in = malloc(NUM_INPUTS * sizeof(int));
  imag_in = malloc(NUM_INPUTS * sizeof(int));
  real_out = malloc(NUM_INPUTS * NUM_INPUTS * sizeof(int));
  imag_out = malloc(NUM_INPUTS * NUM_INPUTS * sizeof(int));

  int input;
  for (input=0; input<NUM_INPUTS; input++) {
    // Subtract NUM_INPUTS/2 so we get values centred around 0.
    real_in[input] = 1;
    imag_in[input] = input - (NUM_INPUTS/2);
  }

  // Set up dataflow network.
  dataflow_config config;
  config.cores = 8;
  config.core_tasks = malloc(config.cores * sizeof(dataflow_func));
  config.core_tasks[0] = &multicast_core0;
  config.core_tasks[1] = &core1;
  config.core_tasks[2] = &core2;
  config.core_tasks[3] = &core3;
  config.core_tasks[4] = &core4;
  config.core_tasks[5] = &core5;
  config.core_tasks[6] = &core6;
  config.core_tasks[7] = &core7;

  // Do work.
  start_dataflow(&config);

  // Print results.
  int i, j;
  for (i=0; i<NUM_INPUTS; i++) {
    for (j=0; j<NUM_INPUTS; j++) {
      printf("(%d + %di)(%d + %di)\t= %d + %di\n",
             real_in[i], imag_in[i],
             real_in[j], imag_in[j],
             real_out[i*NUM_INPUTS + j], imag_out[i*NUM_INPUTS + j]);
    }
  }
  exit(0);
}
