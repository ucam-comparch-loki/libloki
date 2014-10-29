#include "../lokilib.h"
#include <stdio.h>

int array[] = {0,0,0,0,0,0,0,0};

void core_task() {
  int core = get_core_id();
  
  // Receive data over the network.
  int data;
  RECEIVE(data, 7);
  
  // Each core stores a unique value.
  array[core] = core * data;
  
  // All cores except core 0 terminate here.
  if(core != 0)
    asm("or.eop r0, r0, r0\n");
  else {
    int i;
    for(i=0; i<8; i++)
      printf("core %d wrote %d\n", i, array[i]);
    
    // For conciseness, we force a sys_exit, rather than setting up the return
    // addresses properly.
    asm("syscall.eop 1\n");
  }
}

int main(int argc, char** argv) {

  // Give 8 cores a connection to memory and sensible stack/frame pointers.
  init_cores(8);
  
  // Create a bitmask representing which cores we want to send to.
  // bit n set = send to core n (send to same channel on all cores)
  const int bitmask = all_cores(8);
  
  // Convert the bitmask into network addresses.
  const int inst_input = loki_mcast_address(0 /*tile*/, bitmask, 0 /*ipk fifo*/);
  const int data_input = loki_mcast_address(0 /*tile*/, bitmask, 7 /*channel*/);
  
  // Put the addresses in our channel map table.
  SET_CHANNEL_MAP(2, inst_input);
  SET_CHANNEL_MAP(3, data_input);
  
  // Multicast instructions to all cores, telling them to execute core_task().
  asm (
    "rmtexecute -> 2\n"
    "ifp?lli r24, core_task\n"
    "ifp?fetch r24\n"
  );
  
  // Multicast some data to all cores.
  SEND(100000 /*data*/, 3 /*output channel*/);
  
  // Force an end-of-packet, so this core can execute the code it sent to its
  // own FIFO.
  asm("or.eop r0, r0, r0\n");

}
