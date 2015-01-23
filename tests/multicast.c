#include <loki/channels.h>
#include <loki/init.h>
#include <loki/spawn.h>
#include <stdio.h>
#include <stdlib.h>

int array[] = {0,0,0,0,0,0,0,0};

void core_task(const void *data2addr) {
  int core = get_core_id();

  // Receive data over the network.
  int data1 = loki_receive(CH_REGISTER_3);
  int data2 = *(const int *)data2addr;

  // Each core stores a unique value.
  array[core] = core * data1 + data2;
}

int main(int argc, char** argv) {

  // Give 8 cores a connection to memory and sensible stack/frame pointers.
  loki_init_default(8, 0);

  // Create a bitmask representing which cores we want to send to.
  // bit n set = send to core n (send to same channel on all cores)
  const int bitmask = all_cores(8);

  // Convert the bitmask into network addresses.
  const channel_t data_input = loki_mcast_address(0 /*tile*/, bitmask, CH_REGISTER_3);

  int data1 = 100000, data2 = 1300;

  // Put the addresses in our channel map table.
  set_channel_map(3, data_input);
  // Multicast some data to all cores.
  loki_send(3 /*output channel*/, data1 /*data*/);

  distributed_func config;
  config.cores = 8;
  config.func = &core_task;
  config.data = &data2;
  config.data_size = sizeof(data2);

  loki_execute(&config);

  int i;
  for(i=0; i<8; i++) {
    if (array[i] != i * data1 + data2) {
      fprintf(stderr, "Incorrect value: core %d wrote %d not %d\n", i, array[i], i * data1 + data2);
      exit(EXIT_FAILURE);
    }
  }

  exit(EXIT_SUCCESS);
}
