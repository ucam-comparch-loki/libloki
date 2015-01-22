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

#include <loki/channels.h>
#include <loki/patterns/dataflow.h>
#include <loki/init.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_INPUTS 5

int *real_in,  *imag_in;
int *real_out, *imag_out;

void multicast_core0() {
  int bitmask, address;
  // send a to cores 1 and 5, input 3
  bitmask = 0x22;
  address = loki_mcast_address(0, bitmask, 3);
  set_channel_map(2, address);
  // send b to cores 2 and 4, input 3
  bitmask = 0x14;
  address = loki_mcast_address(0, bitmask, 3);
  set_channel_map(3, address);
  // send c to cores 1 and 4, input 4
  bitmask = 0x12;
  address = loki_mcast_address(0, bitmask, 4);
  set_channel_map(4, address);
  // send d to cores 2 and 5, input 4
  bitmask = 0x24;
  address = loki_mcast_address(0, bitmask, 4);
  set_channel_map(5, address);

  // Distribute operands to cores.
  int i, j;
  for (i=0; i<NUM_INPUTS; i++) {
    for (j=0; j<NUM_INPUTS; j++) {
      const int a = real_in[i], b = imag_in[i];
      const int c = real_in[j], d = imag_in[j];

      loki_send(2, a);
      loki_send(3, b);
      loki_send(4, c);
      loki_send(5, d);
    }
  }

  // Wait for finished token from core7.
  loki_receive_token(CH_REGISTER_3);

  end_parallel_section();
}

// a * c
void core1() {
  // Connect to core 3
  loki_connect(2,   0,3,3);

  DATAFLOW_PACKET (1,
    "mullw.eop r0, r3, r4 -> 2\n"
  );
}

// b * d
void core2() {
  // Connect to core 3
  loki_connect(2,   0,3,4);

  DATAFLOW_PACKET (2,
    "mullw.eop r0, r3, r4 -> 2\n"
  );
}

// ac - bd
void core3() {
  // Connect to core 7
  loki_connect(2,   0,7,3);

  DATAFLOW_PACKET (3,
    "subu.eop r0, r3, r4 -> 2\n"
  );
}

// b * c
void core4() {
  // Connect to core 6
  loki_connect(2,   0,6,3);

  DATAFLOW_PACKET (4,
    "mullw.eop r0, r3, r4 -> 2\n"
  );
}

// a * d
void core5() {
  // Connect to core 6
  loki_connect(2,   0,6,4);

  DATAFLOW_PACKET (5,
    "mullw.eop r0, r3, r4 -> 2\n"
  );
}

// bc + ad
void core6() {
  // Connect to core 7
  loki_connect(2,   0,7,4);

  DATAFLOW_PACKET (6,
    "addu.eop r0, r3, r4 -> 2\n"
  );
}

// Store results
void core7() {
  // Connect to core 0
  loki_connect(2, 0, COMPONENT_CORE_0, CH_REGISTER_3);

  DATAFLOW_PACKET_3 (7,
    "addu r25, %1, r0\n"
    "addu r26, %2, r0\n"
    ,
    "stw r3, 0(r25) -> 1\n"
    "stw r4, 0(r26) -> 1\n"
    "addui r25, r25, 4\n"
    "seteq.p r0, r26, %0\n"
    "ifp?or r0, r0, r0 -> 2\n" // Send a finished token
    "addui.eop r26, r26, 4\n"
    ,
    , // outputs
    "r"(imag_out + NUM_INPUTS * NUM_INPUTS - 1) AND "r"(real_out) AND "r"(imag_out), // inputs
    "r11" AND "r12" AND "r13" AND "r14" AND "r15" AND "r16" AND "r17" AND "r18" AND "r19" AND "r20" AND "r21" AND "r22" AND "r24" AND "r25" AND "r26" AND "memory" // clobbers
  );
}

int main(int argc, char** argv) {

  loki_init_default(8, NULL);

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

  // Check results.
  int i, j;
  for (i=0; i<NUM_INPUTS; i++) {
    for (j=0; j<NUM_INPUTS; j++) {
      if (real_in[i] * real_in[j] - imag_in[i] * imag_in[j] != real_out[i*NUM_INPUTS + j]) {
        exit(EXIT_FAILURE);
      }
      if (real_in[i] * imag_in[j] + real_in[j] * imag_in[i] != imag_out[i*NUM_INPUTS + j]) {
        exit(EXIT_FAILURE);
      }
    }
  }
  exit(EXIT_SUCCESS);
}
