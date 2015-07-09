#include <loki/lokilib.h>
#include <assert.h>
#include <stdlib.h>

#define DEFAULT_CREDIT_COUNT 4

//============================================================================//
// Shared
//
//   Various helper functions used by the parallel implementations.
//============================================================================//

enum Environments environment = ENV_NONE;
int environment_version = 0;

static inline void detect_environment(void) {
  int test[2] = { 0, 1 };
  int csim;
  // CSim is old, and easy to fool. It has a bug where it returns the wrong result in this code.
  asm (
    "ldw 0(%1) -> 1\n"
    "ldw 4(%1) -> 1\n"
    "or %0, r2, r0\n"
    "or r0, r2, r0\n"
    : "=r"(csim)
    : "r"(test)
  );
  if (csim) {
    environment = ENV_CSIM;
    return;
  }

  // Easiest way to tell simulators apart at the moment is the channel map table.
  channel_t default_ch2 = get_channel_map(0);
  if (default_ch2 != 0) {
     // All verilog variants initialise channel 2 by default -> we know it's VCS, Verilator or FPGA.
     if (*(int*)0xffc != 0) {
        // The memory wasn't cleared. Must be FPGA.
        environment = ENV_FPGA;
     } else {
        // By design, VCS and Verilator are equivalent. Pick the more likely.
        environment = ENV_VCS;
     }
  } else {
     // Channel 2 wasn't initialised -> lokisim.
     environment = ENV_LOKISIM;
  }
}

static void init_run_config(setup_func func, unsigned int cores) {
  func();
  // Ensure all cores (across all tiles) are done with the config func before returning.
  // This ensures no race conditions exist with configuration functions.
  loki_sync(cores);
}

// More flexible core initialisation. To be executed by core 0 of any tile.
static inline void init_local_tile(const init_config* config) {

  uint tile = get_tile_id();
  uint cores = cores_this_tile(config->cores, tile);

  if (cores <= 1) {
    if (config->config_func != NULL)
      config->config_func();
    return;
  }

  int inst_mem = config->inst_mem;
  int data_mem = config->data_mem;
  uint stack_size = config->stack_size;
  uint stack_pointer = (uint)config->stack_pointer - tile2int(tile)*CORES_PER_TILE*stack_size;
  channel_t inst_mcast = loki_mcast_address(all_cores_except_0(cores), 0, false);
  channel_t data_mcast = loki_mcast_address(all_cores_except_0(cores), 7, false);

  set_channel_map(10, inst_mcast);
  set_channel_map(11, data_mcast);

  loki_send(11, inst_mem);
  loki_send(11, data_mem);
  loki_send(11, stack_pointer);
  loki_send(11, stack_size);

  asm volatile (
    "fetchr 0f\n"
    "rmtexecute -> 10\n"        // begin remote execution
    "cregrdi r11, 1\n"          // get core id, and put into r11
    "andi r11, r11, 0x7\n"      // get core id, and put into r11
    "slli r23, r11, 2\n"        // the memory port to connect to
    "addu r17, r7, r23\n"       // combine port with received inst_mem
    "addu r18, r7, r23\n"       // combine port with received data_mem
    "setchmapi 0, r17\n"
    "setchmapi 1, r18\n"
    "or r8, r7, r0\n"           // receive stack pointer
    "mullw r14, r7, r11\n"      // multiply core ID by stack size
    "subu r8, r8, r14\n"
    "or.eop r9, r8, r0\n"       // frame pointer = stack pointer
    "0:\n"
  );

  if (config->config_func != NULL) {
    loki_send(11, (int)&loki_sleep); // return address
    loki_send(11, (int)config->config_func); // arg1
    loki_send(11, (int)config->cores); // arg2
    loki_send(11, (int)&init_run_config); // function

    asm volatile (
      "fetchr 0f\n"
      "rmtexecute -> 10\n"      // begin remote execution
      "or r10, r7, r0\n"        // set return address
      "or r13, r7, r0\n"        // set arg1
      "or r14, r7, r0\n"        // set arg2
      "fetch.eop r7\n"          // fetch function to perform further init
      "0:\n"
    );

    // run the same code as other cores.
    init_run_config(config->config_func, config->cores);
  } else {
    // Since there is no config func, all cores are ready at this point. Just need a tile to tile sync.
    loki_sync_tiles(num_tiles(config->cores));
  }
}

// Make a copy of the configuration struct in this tile, as it is awkward/
// impossible to share it via memory.
void receive_init_config() {
  init_config* config = (init_config*)loki_receive(7);
  loki_receive_data(config, sizeof(init_config), 7);

  init_local_tile(config);
}

static inline void init_remote_tile(const tile_id_t tile, const init_config* config) {

  // Connect to core 0 in the tile.
  int inst_fifo = loki_core_address(tile, 0, 0, DEFAULT_CREDIT_COUNT);
  int data_input = loki_core_address(tile, 0, 7, DEFAULT_CREDIT_COUNT);
  set_channel_map(10, inst_fifo);
  set_channel_map(11, data_input);

  // Initialise core 0 (stack, memory connections, etc)
  loki_send(11, config->inst_mem);
  loki_send(11, config->data_mem);
  loki_send(11, (int)config->stack_pointer - tile2int(tile)*CORES_PER_TILE*config->stack_size);
  loki_send(11, (int)&loki_sleep);
  
  // Still have to send function pointers, but only have 4 buffer spaces.
  // Wait until after sending instructions to send more data.

  asm (
    "fetchr 0f\n"
    "rmtexecute -> 10\n"        // begin remote execution
    "setchmapi 0, r7\n"         // instruction channel
    "setchmapi 1, r7\n"         // data channel
    "or r8, r7, r0\n"           // receive stack pointer
    "or r9, r8, r0\n"           // frame pointer = stack pointer
    "or r10, r7, r0\n"          // set return address
    "fetch.eop r7\n"            // fetch receive_init_config
    "0:\n"
  );

  loki_send(11, (int)&receive_init_config);
  
  init_config* remotePointer = malloc(sizeof(init_config));
  assert(remotePointer != NULL);
  loki_send(11, (int)remotePointer);
  loki_send_data(config, sizeof(init_config), 11);

}

inline void loki_init(init_config* config) {
  assert(config->cores > 0);
  
  if (environment == ENV_NONE)
    detect_environment();

  if (config->stack_pointer == 0) {
    char *stack_pointer; // Core 0's default stack pointer
    asm ("addu %0, r8, r0\nfetchr.eop 0f\n0:\n" : "=r"(stack_pointer) : : );
    stack_pointer += 0x400 - ((int)stack_pointer & 0x3ff); // Guess at the initial sp

    config->stack_pointer = stack_pointer;
  }

  // Instruction channel
  if (config->inst_mem == 0)
    config->inst_mem = get_channel_map(0);
  // Data channel
  if (config->data_mem == 0)
    config->data_mem = get_channel_map(1);

  // Give each core connections to memory and a stack.
  if (config->cores > 1) {
    unsigned int tile;
    for (tile = 1; tile*CORES_PER_TILE < config->cores; tile++) {
      init_remote_tile(int2tile(tile), config);
    }

    init_local_tile(config);
  }
  else if (config->config_func != NULL)
    config->config_func();
}

// A wrapper for loki_init which fills in most of the values with sensible
// defaults.
void loki_init_default(const uint cores, const setup_func setup) {

  init_config *config = malloc(sizeof(init_config));
  config->cores = cores;
  config->stack_pointer = 0;
  config->stack_size = 0x12000;
  config->inst_mem = 0;
  config->data_mem = 0;
  config->config_func = setup;

  loki_init(config);
  
  free(config);

}

// Get a core to execute a function. The remote core must already have all of
// the necessary function arguments in the appropriate registers.
void loki_remote_execute(void* func, int core) {
  const channel_t ipk_fifo = loki_mcast_address(single_core_bitmask(core), 0, false);
  const channel_t data_input = loki_mcast_address(single_core_bitmask(core), 7, false);
  set_channel_map(2, ipk_fifo);
  set_channel_map(3, data_input);

  // Send the function address to the remote core.
  loki_send(3, (int)func);

  // Tell the remote core to fetch the provided function, and sleep when done.
  asm (
    "fetchr 0f\n"
    "rmtexecute -> 2\n"
    "lli r10, %%lo(loki_sleep)\n"
    "lui r10, %%hi(loki_sleep)\n"
    "fetch.eop r7\n"
    "0:\n"
    :::
  );
}

// Signal that all required results have been produced by the parallel execution
// pattern, and that we may now break the cores from their infinite loops.
void end_parallel_section() {

  // Current implementation is to send a token to core 0, input 7.
  // wait_end_parallel_section() must therefore execute on core 0.
  int address = loki_mcast_address(single_core_bitmask(0), 7, false);
  set_channel_map(2, address);
  loki_send_token(2);

}

// Only continue after all tiles have executed this function. Tokens from each
// tile are collected, then redistributed to show when all have been received.
void loki_sync_tiles(const uint tiles) {
  if (tiles <= 1)
    return;

  assert(get_core_id() == 0);
  uint tile = tile2int(get_tile_id());

  // All tiles except the final one wait until they receive a token from their
  // neighbour. (A tree structure would be more efficient, but there isn't much
  // difference with so few tiles.)
  if (tile < tiles-1)
    loki_receive_token(5);

  // All tiles except the first one send a token to their other neighbour
  // (after setting up a connection).
  if (tile > 0) {
    int address = loki_core_address(int2tile(tile-1), 0, 5, DEFAULT_CREDIT_COUNT);
    set_channel_map(10, address);
    loki_send_token(10);
    loki_receive_token(5);
  } else {
    // All tokens have now been received, so notify all tiles.
    assert(tile == 0);
    int destination;
    for (destination = 1; destination < tiles; destination++) {
      int address = loki_core_address(int2tile(destination), 0, 5, DEFAULT_CREDIT_COUNT);
      set_channel_map(10, address);
      loki_send_token(10);
    }
  }
}

// Only continue after all cores have executed this function. Tokens from each
// core are collected, then redistributed to show when all have been received.
// TODO: could speedup sync within a tile by using multicast until buffers fill?
void loki_sync(const uint cores) {
  if (cores <= 1)
    return;

  uint core = get_core_id();
  tile_id_t tile = get_tile_id();
  uint coresThisTile = cores_this_tile(cores, tile);

  // All cores except the final one wait until they receive a token from their
  // neighbour. (A tree structure would be more efficient, but there isn't much
  // difference with so few cores.)
  if (core < coresThisTile-1)
    loki_receive_token(6);

  // All cores except the first one send a token to their other neighbour
  // (after setting up a connection).
  if (core > 0) {
    int address = loki_mcast_address(single_core_bitmask(core-1), 6, false);
    set_channel_map(10, address);
    loki_send_token(10);
    
    // Receive token from core 0, telling us that synchronisation has finished.
    loki_receive_token(6);
  } else {
    // All core 0s then synchronise between tiles using the same process.
    assert(core == 0);

    loki_sync_tiles(num_tiles(cores));

    // All core 0s need to distribute the token throughout their tiles.
    if (coresThisTile > 1) {
      int bitmask = all_cores_except_0(coresThisTile);
      int address = loki_mcast_address(bitmask, 6, false);
      set_channel_map(10, address);
      loki_send_token(10);
    }
  }
}

void loki_tile_sync(const uint cores) {
  assert(cores <= CORES_PER_TILE);

  if (cores <= 1)
    return;

  uint core = get_core_id();

  // All cores except the final one wait until they receive a token from their
  // neighbour. (A tree structure would be more efficient, but there isn't much
  // difference with so few cores.)
  if (core < cores-1)
    loki_receive_token(6);

  // All cores except the first one send a token to their other neighbour
  // (after setting up a connection).
  if (core > 0) {
    int address = loki_mcast_address(single_core_bitmask(core-1), 6, false);
    set_channel_map(10, address);
    loki_send_token(10);
    loki_receive_token(6);
  }

  // Core 0 notifies all other cores when synchronisation has finished.
  if (core == 0) {
    // All core 0s need to distribute the token throughout their tiles.
    int bitmask = all_cores_except_0(cores);
    int address = loki_mcast_address(bitmask, 6, false);
    set_channel_map(10, address);
    loki_send_token(10);
  }
}

// Wait until the end_parallel_section function has been called. This must be
// executed on core 0.
inline void wait_end_parallel_section() {
  loki_receive_token(7);
}

//============================================================================//
// Distributed execution
//
//   All cores execute the same function, which may itself choose different
//   roles for different cores.
//============================================================================//

// Start cores on the current tile. This must be executed by core 0.
void distribute_to_local_tile(const distributed_func* config) {

  // Make multicast connections to all other members of the SIMD group.
  const tile_id_t tile = get_tile_id();
  const int cores = cores_this_tile(config->cores, tile);

  if (cores > 1) {
    const unsigned int bitmask = all_cores_except_0(cores);
    const channel_t ipk_fifos = loki_mcast_address(bitmask, 0, false);
    const channel_t data_inputs = loki_mcast_address(bitmask, 7, false);

    set_channel_map(10, ipk_fifos);
    set_channel_map(11, data_inputs);
    loki_send(11, (int)config->data);      // send pointer to function argument(s)
    loki_send(11, (int)&loki_sleep);       // send function pointers
    loki_send(11, (int)config->func);

    // Tell all cores to start executing the loop.
    asm volatile (
      "fetchr 0f\n"
      "rmtexecute -> 10\n"        // begin remote execution
      "addu r13, r7, r0\n"        // receive pointer to arguments
      "addu r10, r7, r0\n"        // set return address
      "fetch.eop r7\n"            // fetch function to execute
      "0:\n"
      // No clobbers because this is all executed remotely.
    );
  }

  // Now that all the other cores are going, this core can start on its share of
  // the work.
  config->func(config->data);

}

// Make a copy of the configuration struct in this tile, as it is awkward/
// impossible to share it via memory.
void receive_config() {
  distributed_func* config = malloc(sizeof(distributed_func));
  //config = loki_receive(7);
  void* val;
  val = (void *)loki_receive(7);
  config->cores = (int)val;
  val = (void *)loki_receive(7);
  config->func = val;
  val = (void *)loki_receive(7);
  config->data_size = (size_t)val;
  void *data = malloc((size_t)val);
  config->data = data;
  //val = (void *)loki_receive(7);
  //config->data = val;

  loki_receive_data(data, config->data_size, 7);

  distribute_to_local_tile(config);
}

// Start cores on another tile.
void distribute_to_remote_tile(tile_id_t tile, const distributed_func* config) {

  // Connect to core 0 in the tile.
  channel_t inst_fifo = loki_core_address(tile, 0, 0, DEFAULT_CREDIT_COUNT);
  channel_t data_input = loki_core_address(tile, 0, 7, DEFAULT_CREDIT_COUNT);
  set_channel_map(10, inst_fifo);
  set_channel_map(11, data_input);

  loki_send(11, (int)&loki_sleep);            // send function pointers
  loki_send(11, (int)&receive_config);

  // Tell core 0 to receive configuration struct before setting up all other
  // cores on its tile.
  asm volatile (
    "fetchr 0f\n"
    "rmtexecute -> 10\n"        // begin remote execution
    "addu r10, r7, r0\n"        // set return address
    "fetch.eop r7\n"            // fetch function to execute
    "0:\n"
    // No clobbers because this is all executed remotely.
  );

  //loki_send(11, malloc(sizeof(distributed_func)));
  loki_send(11, config->cores);
  loki_send(11, (int)config->func);
  loki_send(11, config->data_size);
  //loki_send(11, malloc(config->data_size));

  loki_send_data(config->data, config->data_size, 11);

}

// The main function to call to execute the same function on many cores.
void loki_execute(const distributed_func* config) {

  if (config->cores > 1) {
    int tile;
    for (tile = 1; tile*CORES_PER_TILE < config->cores; tile++) {
      distribute_to_remote_tile(int2tile(tile), config);
    }

    distribute_to_local_tile(config);
  }
  else
    config->func(config->data);

}



//============================================================================//
// SIMD
//
//   All cores execute the same code, but not in lockstep. Each core executes
//   1/num_cores of the iterations, currently strided (i.e. next iteration =
//   current iteration + num cores).
//   TODO: option to use chunked distribution (requires a division)
//============================================================================//

// Signal that this core has finished its share of the SIMD loop. This is done
// in such a way that when the control core (the one that started the SIMD stuff)
// receives the signal, it knows that all cores have finished and that it is
// safe to continue through the program.
// TODO: it may be worth combining this step with the reduction stage - e.g.
// instead of sending a token, send this core's partial result.
inline void simd_finished(const loop_config* config, int core) {

  // All cores except the final one wait until they receive a token from their
  // neighbour. (A tree structure would be more efficient, but there isn't much
  // difference with so few cores.)
  if (core < config->cores - 1)
    loki_receive_token(6);

  // All cores except the first one send a token to their other neighbour
  // (after setting up a connection).
  if (core > 0) {
    int address = loki_mcast_address(single_core_bitmask(core-1), 6, false);
    set_channel_map(2, address);
    loki_send_token(2);
  }

}

// The code that a single core in the SIMD array executes. Note that the
// iterations are currently striped across the cores - this avoids needing a
// division to find out which cores execute which iterations, but may reduce
// locality.
void worker_core(const loop_config* config, int core) {

  int cores = config->cores;
  int iterations = config->iterations;
  iteration_func func = config->iteration;

  // Perform any initialisation if necessary.
  if (config->initialise != NULL)
    config->initialise(cores, iterations, core);

  int iter;
  if (config->helper == NULL) {
    for (iter = core; iter < iterations; iter += cores) {
      // Does the compiler have to assume that all non-volatile registers may be
      // written to? It would then have to store/retrieve them all every
      // iteration, which is bad for tight loops.
      func(iter, core);
    }
  }
  else {
    int do_iteration;
    iter = core-1;  // Helper core at position 0 doesn't execute iterations.

    do_iteration = loki_receive(7);
    while (do_iteration) {
      func(iter, core-1);

      iter += cores-1;
      do_iteration = loki_receive(7);
    }
  }

  // Perform any tidying if necessary (e.g. drain buffers).
  if (config->tidy != NULL)
    config->tidy(cores, iterations, core);

  // Signal that this core has finished its work. Could this happen before
  // tidying?
  simd_finished(config, core);
}

// A core will stop work if it executes this function.
void loki_sleep() {
  asm (
    "or.eop r0, r0, r0\n"
  );
}

// Might need inline assembly for lots of this to reduce overhead of sending/receiving
void helper_core(const loop_config* config) {
  assert(config->cores >= 2);

  int total_iterations = config->iterations;
  int cores = config->cores;
  int simd_cores = cores - 1; // subset of cores executing SIMD code
  helper_func func = config->helper;

  // Connect to all cores.
  int bitmask = all_cores_except_0(cores);
  int address = loki_mcast_address(bitmask, 7, false);
  set_channel_map(10, address);

  if (config->helper_init != NULL)
    config->helper_init();

  int iterations;
  for (iterations = 0; iterations+simd_cores < total_iterations; iterations += simd_cores) {
    loki_send(10, 1); // Would prefer to combine this with the branch instruction.

    // Perform any computation shared by all cores and send results.
    func();
  }

  // Complete any final iterations for which not all cores are needed.
  if (iterations != total_iterations) {
    int iterations_remaining = total_iterations - iterations;

    if (iterations_remaining > 0) {
      int bitmask1 = all_cores_except_0(iterations_remaining + 1); // +1 accounts for helper core
      int address1 = loki_mcast_address(bitmask1, 7, false);
      set_channel_map(10, address1);

      // Send 1 to cores in bitmask1
      // Perform computation shared by all cores and send results.
      loki_send(10, 1);
      func();
    }
  }

  // Send 0 to all cores so they know to stop.
  set_channel_map(10, address);
  loki_send(10, 0);

  // Signal that this core has finished its work. Do we need to tidy() too?
  simd_finished(config, 0);
}

void simd_member(const loop_config* config, const int core) {
  const int tile = tile2int(get_tile_id());

  if (core == 0) {
    // Determine which role this core is going to play.
    if (config->helper != NULL)
      helper_core(config);
    else
      worker_core(config, core + 8*tile);

    // Combine each core's partial result before returning.
    if (config->reduce != NULL)
      config->reduce(config->cores);
  }
  else {
    worker_core(config, core + 8*tile);
  }
}

// Set up a SIMD group on the current tile. This must be executed by core 0.
void simd_local_tile(const loop_config* config) {

  // Make multicast connections to all other members of the SIMD group.
  const tile_id_t tile = get_tile_id();
  const int cores = cores_this_tile(config->cores, tile);
  const unsigned int bitmask = all_cores_except_0(cores);
  const channel_t ipk_fifos = loki_mcast_address(bitmask, 0, false);
  const channel_t data_inputs = loki_mcast_address(bitmask, 7, false);

  set_channel_map(10, ipk_fifos);
  set_channel_map(11, data_inputs);
  loki_send(11, (int)config);            // send pointer to configuration info
  loki_send(11, (int)&loki_sleep);       // send function pointers
  loki_send(11, (int)&simd_member);

  // Tell all cores to start executing the loop.
  asm volatile (
    "fetchr 0f\n"
    "rmtexecute -> 10\n"        // begin remote execution
    "addu r13, r7, r0\n"        // receive pointer to configuration info
    "cregrdi r11, 1\n"          // get core id, and put into r11
    "andi r11, r11, 0x7\n"      // get core id, and put into r11
    "addu r14, r11, r0\n"       // put core position in argument-passing register
    "addu r10, r7, r0\n"        // set return address
    "fetch.eop r7\n"            // fetch function to execute
    "0:\n"
    // No clobbers because this is all executed remotely.
  );

}

// Set up a SIMD group on another tile.
void simd_remote_tile(const tile_id_t tile, const loop_config* config) {

  // Connect to core 0 in the tile.
  channel_t inst_fifo = loki_core_address(tile, 0, 0, DEFAULT_CREDIT_COUNT);
  channel_t data_input = loki_core_address(tile, 0, 7, DEFAULT_CREDIT_COUNT);
  set_channel_map(10, inst_fifo);
  set_channel_map(11, data_input);

  loki_send(11, (int)config);                 // send pointer to configuration info
  loki_send(11, (int)&loki_sleep);            // send function pointers
  loki_send(11, (int)&simd_local_tile);

  // Tell core 0 to set up all other cores on its tile.
  // TODO: will the pointer to data work from the other tile?
  asm volatile (
    "fetchr 0f\n"
    "rmtexecute -> 10\n"        // begin remote execution
    "addu r13, r7, r0\n"        // receive pointer to configuration info
    "addu r10, r7, r0\n"        // set return address
    "fetch.eop r7\n"            // fetch function to execute
    "0:\n"
    // No clobbers because this is all executed remotely.
  );

}

// The main function to call to execute the loop in parallel.
void simd_loop(const loop_config* config) {

  if (config->cores > 1) {
    int tile;
    for (tile = 1; tile < num_tiles(config->cores); tile++) {
      simd_remote_tile(int2tile(tile), config);
    }

    simd_local_tile(config);
  }

  // Now that all the other cores are going, this core can start on its share of
  // the work.
  simd_member(config, 0);

}


//============================================================================//
// Worker farm
//
//   One core acts as the master and distributes work across a group of workers.
//   The work is issued in the form of a loop iteration index.
//============================================================================//

// Send a message from a worker to the master, asking for the next iteration.
inline void request_work() {
  loki_send(2, get_core_id());
}

// The loop executed by each worker. Executes the provided function for as long
// as the master allows the worker to live (a nxipk command will be sent when
// all iterations have completed).
void worker_thread(const loop_config* config, const int worker) {

  // Create a connection back to the master core.
  // The master core has the following inputs reserved:
  //   0: instruction packet FIFO
  //   1: instruction packet cache
  //   2: channel used to communicate with memory

  int address = loki_mcast_address(single_core_bitmask(0), get_core_id() + 2, false);
  set_channel_map(2, address);


  // Loop forever, executing the iterations provided. The master will kill the
  // worker by sending -1 as the iteration.
  while (1) {
    request_work();

    // Receive the iteration to execute next.
    int iteration;
    iteration = loki_receive(7);

    if (iteration == -1) break; // end of work signal
    // Execute the loop iteration.
    config->iteration(iteration, worker);
  }
}

// Distribute all loop iterations across the workers, and wait for them all to
// complete.
void worker_farm(const loop_config* config) {
  assert(config->cores > 1);
  assert(config->cores <= 6); // Currently limited by number of master's input ports

  // Make multicast connections to all workers.
  unsigned int bitmask = all_cores_except_0(config->cores);
  const channel_t ipk_fifos = loki_mcast_address(bitmask, 0, false);
  const channel_t data_inputs = loki_mcast_address(bitmask, 7, false);
  set_channel_map(2, ipk_fifos);
  set_channel_map(3, data_inputs);

  loki_send(3, (int)config);  // send pointer to configuration info

  asm (
    "fetchr 0f\n"
    "rmtexecute -> 2\n"             // begin remote execution
    "addu r13, r7, r0\n"            // receive pointer to configuration info
    "cregrdi r11, 1\n"              // get core id, and put into r11
    "andi r11, r11, 0x7\n"          // get core id, and put into r11
    "addui r14, r11, -1\n"          // worker ID = core ID - 1 because core 0 is master
    "lli r10, %lo(loki_sleep)\n"    // set return address - sleep when finished
    "lui r10, %hi(loki_sleep)\n"
    "lli r24, %lo(worker_thread)\n"
    "lui r24, %hi(worker_thread)\n"
    "fetch.eop r24\n"               // fetch the worker's task
    "0:\n"
  );

  // Issue workers with loop iterations to work on.
  int iter;
  for (iter = 0; iter < config->iterations; iter++) {
    const int worker = receive_any_input();             // wait for any worker to request work
    const int worker_addr = loki_mcast_address(single_core_bitmask(worker), 7, false);
    set_channel_map(3, worker_addr);                    // connect
    loki_send(3, iter);                                      // send work
  }

  // Wait for all workers to finish their final tasks, and kill them.
  int finished;
  for (finished = 0; finished < config->cores-1; finished++) {
    const int worker = receive_any_input();             // wait for any worker to request work
    const int worker_addr = loki_mcast_address(single_core_bitmask(worker), 7, false);
    set_channel_map(3, worker_addr);                    // connect
    loki_send(3, -1);                                      // send end of work
  }

  // Combine each core's partial result before returning.
  config->reduce(config->cores-1);

}


//============================================================================//
// Task-level pipeline
//
//   Each core executes a different task, and tells the next core when it has
//   finished each iteration.
//============================================================================//

void pipeline_stage(const pipeline_config* config, const int stage) {

  const int have_predecessor = (stage > 0);
  const int have_successor = (stage < config->cores - 1);

  // Address to connect to. Most cores connect to the next stage in the pipeline
  // so they can tell when it is safe to begin the next loop iteration. The
  // final core, however, tells the first core when all work has finished.
  int next_addr;
  if (have_successor)
    next_addr = loki_mcast_address(single_core_bitmask(stage+1), 6, false);
  else
    next_addr = loki_mcast_address(single_core_bitmask(0), 6, false);

  // Make connection.
  set_channel_map(2, next_addr);

  // If there is work to do before the pipeline starts, do it now.
  if (config->initialise && config->initialise[stage])
    config->initialise[stage]();

  // Execute loop
  int i;
  for (i=0; i<config->iterations; i++) {
    // If there is a previous core in the pipeline, wait for it to tell us that
    // we can begin work on the next iteration.
    if (have_predecessor)
      loki_receive_token(6);

    // Execute this iteration.
    config->stage_func[stage](i);

    // If there is a subsequent core in the pipeline, tell it that it may now
    // begin work on the next iteration.
    if (have_successor)
      loki_send_token(2);
  }

  // Final core tells core 0 when all work has finished.
  if (!have_successor)
    loki_send_token(2);

  // Core 0 only returns when it receives the signal from the final core.
  if (!have_predecessor)
    loki_receive_token(6);

  // Release any resources when the pipeline has finished.
  // TODO: can cores other than 0 ever reach this code?
  if (config->tidy && config->tidy[stage])
    config->tidy[stage]();

}

void pipeline_loop(const pipeline_config* config) {

  // Tell all cores to start executing the loop.
  const int bitmask = all_cores_except_0(config->cores);
  const channel_t ipk_fifo = loki_mcast_address(bitmask, 0, false);
  const channel_t data_input = loki_mcast_address(bitmask, 7, false);
  set_channel_map(2, ipk_fifo);
  set_channel_map(3, data_input);

  loki_send(3, (int)config);

  asm (
    "fetchr 0f\n"
    "rmtexecute -> 2\n"             // begin remote execution
    "addu r13, r7, r0\n"            // receive pointer to configuration info
    "cregrdi r11, 1\n"              // get core id, and put into r11
    "andi r11, r11, 0x7\n"          // get core id, and put into r11
    "addu r14, r11, r0\n"
    "lli r10, %lo(loki_sleep)\n"    // set return address - sleep when finished
    "lui r10, %hi(loki_sleep)\n"    // set return address - sleep when finished
    "lli r24, %lo(pipeline_stage)\n"
    "lui r24, %hi(pipeline_stage)\n"
    "fetch.eop r24\n"               // fetch the pipeline stage's task
    "0:\n"
  );

  // Also start this core.
  pipeline_stage(config, 0);

}

void dd_pipeline_stage(const dd_pipeline_config* config, const int stage) {

  const int have_successor = (stage < config->cores - 1);

  // Address to connect to. Most cores connect to the next stage in the pipeline
  // so they can tell when it is safe to begin the next loop iteration. The
  // final core, however, tells the first core when all work has finished.
  int next_addr;
  if (have_successor)
    next_addr = loki_mcast_address(single_core_bitmask(stage+1), 6, false);
  else
    next_addr = loki_mcast_address(single_core_bitmask(0), 6, false);

  // Make connection.
  set_channel_map(2, next_addr);

  // If there is work to do before the pipeline starts, do it now.
  if (config->initialise && config->initialise[stage])
    config->initialise[stage]();

  dd_pipeline_func my_func = config->stage_tasks[stage];

  // Stage 0 is in charge of supplying the data to the pipeline, so it should
  // know how much there is and how many iterations to perform.
  if (stage == 0) {
    int arg = 0;
    while (1) {
      int result = my_func(arg);
      if (result == config->end_of_stream_token) {
        if (have_successor)
          loki_send(2, result);
        break;
      }
      arg++;
    }
  }
  // All other cores enter an infinite loop, working with the data they receive
  // for as long as it arrives. At some point one core must call the
  // end_parallel_section function.
  else {
    while (1) {
      // Receive an argument from the previous core in the pipeline. (Assuming
      // there is only one argument - more can be passed manually, however.)
      int arg;
      arg = loki_receive(6);

      if (arg == config->end_of_stream_token) {
        if (have_successor)
          loki_send(2, arg);
        break;
      }

      // Execute this iteration.
      int result = my_func(arg);

      // If there is a subsequent core in the pipeline, send it an argument to
      // work on.
      if (have_successor)
        loki_send(2, result);
    }
  }

  // Release any resources when the pipeline has finished.
  if (config->tidy && config->tidy[stage])
    config->tidy[stage]();

  // Signal that the pipeline has finished (assumes that no one has a long tidy
  // function).
  if (!have_successor)
    end_parallel_section();

}

void dd_pipeline_loop(const dd_pipeline_config* config) {

  if (config->cores > 1) {
    // Tell all cores to start executing the loop.
    const int bitmask = all_cores_except_0(config->cores);
    const channel_t ipk_fifo = loki_mcast_address(bitmask, 0, false);
    const channel_t data_input = loki_mcast_address(bitmask, 7, false);
    set_channel_map(2, ipk_fifo);
    set_channel_map(3, data_input);

    loki_send(3, (int)config);

    asm (
      "fetchr 0f\n"
      "rmtexecute -> 2\n"           // begin remote execution
      "addu r13, r7, r0\n"          // receive pointer to configuration info
      "cregrdi r11, 1\n"            // get core id, and put into r14
      "andi r14, r11, 0x7\n"        // get core id, and put into r14
      "lli r10, %lo(loki_sleep)\n"  // set return address - sleep when finished
      "lui r10, %hi(loki_sleep)\n"  // set return address - sleep when finished
      "lli r24, %lo(dd_pipeline_stage)\n"
      "lui r24, %hi(dd_pipeline_stage)\n"
      "fetch.eop r24\n"             // fetch the pipeline stage's task
      "0:\n"
    );
  }

  // Also start this core.
  dd_pipeline_stage(config, 0);


  if (config->cores > 1) {
    // Wait for the rest of the pipeline to finish before continuing. Would like
    // to remove this stall, but there are issues if we start sending
    // instructions to FIFOs when other cores are still doing work.
    wait_end_parallel_section();
  }

}


//============================================================================//
// Dataflow
//
//   Each core repeatedly executes a single instruction packet, receiving data
//   over the network and sending results on to one or more cores.
//============================================================================//

void start_dataflow(const dataflow_config* config) {

  // Send each core the function it is to execute.
  int core;
  for (core = 1; core < config->cores; core++)
    loki_remote_execute(config->core_tasks[core], core);

  // Once all other cores have been set up, this core can join in.
  config->core_tasks[0]();

  // After this core has finished its work, wait for the dataflow network to
  // drain, then kill all cores.
  wait_end_parallel_section();

  const int bitmask = all_cores_except_0(config->cores);
  const int ipk_fifos = loki_mcast_address(bitmask, 0, false);
  set_channel_map(2, ipk_fifos);
  loki_send_interrupt(2);

  // The dataflow macro stores the address of any tidy-up code which needs to
  // be executed when dataflow has finished. Tell the core to fetch this now.
  asm (
    "fetchr 0f\n"
    "rmtexecute -> 2\n"
    "fetch.eop r24\n"
    "0:\n"
  );

}


//============================================================================//
// Other
//============================================================================//

#include <stdarg.h>

// Function for the remote core to execute to receive all required arguments.
void spawn_prep() {

  int return_address;
  int (*func)(int r13, int r14, int r15, int r16, int r17);
  int args[5];

  return_address = loki_receive(7);
  set_channel_map(2, return_address);

  func = (void *)loki_receive(7);

  int argc = loki_receive(7);
  int i;
  assert(argc <= 5);
  for (i = 0; i < argc; i++) {
    args[i] = loki_receive(7);
  }

  // Calling func like this assumes nothing bad happens if the function doesn't use the later arguments.
  // This is a reasonable assumption for 5 arguments, but beyond that it's a bit dodgy.
  loki_send(2, func(args[0], args[1], args[2], args[3], args[4]));
}

// Execute a function on a remote core, and return its result to a given place.
void loki_spawn(void* func, const channel_t address, const int argc, ...) {
  // Find a free core and connect to it
  int core = 1;

  // r13-r17
  assert(argc <= 5);

  // Tell the remote core to execute a setup function which will allow it to
  // receive all arguments for the main function.
  loki_remote_execute(&spawn_prep, core);

  // Send all information required to execute the function remotely.
  va_list argp;
  va_start(argp, argc); // argc = last fixed argument
  loki_send(3, address);
  loki_send(3, (int)func);
  loki_send(3, argc);

  int i;
  for (i=0; i<argc; i++) {
    int arg = va_arg(argp, int);
    loki_send(3, arg);
  }

  va_end(argp);

}
