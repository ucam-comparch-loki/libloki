#include <loki/lokilib.h>
#include <assert.h>
#include <stdlib.h>

//============================================================================//
// Shared
//
//   Various helper functions used by the parallel implementations.
//============================================================================//

// More flexible core initialisation. To be executed by core 0 of any tile.
static inline void init_local_tile(const init_config* config) {

  uint tile = get_tile_id();
  uint cores = cores_this_tile(config->cores, tile);

  if (cores <= 1) {
    if (config->config_func != NULL)
      config->config_func();
    return;
  }

  int inst_mem = config->inst_mem | (tile << 20);
  int data_mem = config->data_mem | (tile << 20);
  uint stack_size = config->stack_size;
  uint stack_pointer = config->stack_pointer - tile*CORES_PER_TILE*stack_size;
  int inst_mcast = loki_mcast_address(tile, all_cores_except_0(cores), 0);
  int data_mcast = loki_mcast_address(tile, all_cores_except_0(cores), 7);

  SET_CHANNEL_MAP(10, inst_mcast);
  SET_CHANNEL_MAP(11, data_mcast);

  SEND(inst_mem, 11);
  SEND(data_mem, 11);
  SEND(stack_pointer, 11);
  SEND(stack_size, 11);
  // Still have to send function pointer, but only have 4 buffer spaces.
  // Wait until after sending instructions to send more data.

  asm volatile (
    "rmtexecute -> 10\n"            // begin remote execution
    "ifp?cregrdi r11, 1\n"          // get core id, and put into r11
    "ifp?andi r11, r11, 0x7\n"      // get core id, and put into r11
    "ifp?slli r23, r11, 8\n"        // the memory port to connect to
    "ifp?addu r17, r7, r23\n"       // combine port with received inst_mem
    "ifp?addu r18, r7, r23\n"       // combine port with received data_mem
    "ifp?setchmapi 0, r17\n"
    "ifp?setchmapi 1, r18\n"
    "ifp?or r8, r7, r0\n"           // receive stack pointer
    "ifp?mullw r14, r7, r11\n"      // multiply core ID by stack size
    "ifp?subu r8, r8, r14\n"
    "ifp?or r9, r8, r0\n"           // frame pointer = stack pointer
    "ifp?or r10, r7, r0\n"          // set return address
//    "ifp?fetch r7\n"                // fetch function to perform further init
    "nor r0, r0, r0\n"              // NOP just in case compiler puts an ifp? instruction next.
  );

  SEND(&loki_sleep, 11);

  if (config->config_func != NULL) {
    asm volatile (
      "rmtexecute -> 10\n"          // begin remote execution
      "ifp?fetch r7\n"              // fetch function to perform further init
      "nor r0, r0, r0\n"              // NOP just in case compiler puts an ifp? instruction next.
    );

    SEND(config->config_func, 11);
    config->config_func();
  }
}

// Make a copy of the configuration struct in this tile, as it is awkward/
// impossible to share it via memory.
void receive_init_config() {
  init_config* config = malloc(sizeof(init_config));
  RECEIVE_STRUCT(config, sizeof(init_config), 7);

  init_local_tile(config);
}

static inline void init_remote_tile(const uint tile, const init_config* config) {

  // Connect to core 0 in the tile.
  int inst_fifo = loki_core_address(tile, 0, 0);
  int data_input = loki_core_address(tile, 0, 7);
  SET_CHANNEL_MAP(10, inst_fifo);
  SET_CHANNEL_MAP(11, data_input);

  // Initialise core 0 (stack, memory connections, etc)
  SEND(config->inst_mem, 11);
  SEND(config->data_mem, 11);
  SEND(config->mem_config, 11);

  SEND(config->stack_pointer - tile*CORES_PER_TILE*config->stack_size, 11);
  // Still have to send function pointers, but only have 4 buffer spaces.
  // Wait until after sending instructions to send more data.

  asm (
    "rmtexecute -> 10\n"            // begin remote execution
    "ifp?cregrdi r11, 1\n"          // get tile id, and put into r11
    "ifp?srli r11, r11, 4\n"        // get tile id, and put into r11
    "ifp?slli r23, r11, 20\n"       // the memory port to connect to
    "ifp?addu r17, r7, r23\n"       // combine port with received inst_mem
    "ifp?addu r18, r7, r23\n"       // combine port with received data_mem
    "ifp?setchmapi 0, r17\n"
    "ifp?setchmapi 1, r18\n"
    "ifp?or r0, r7, r0 -> 0\n"      // send memory configuration command
    "ifp?or r8, r7, r0\n"           // receive stack pointer
    "ifp?or r9, r8, r0\n"           // frame pointer = stack pointer
    "ifp?or r10, r7, r0\n"          // set return address
    "ifp?fetch r7\n"                // fetch receive_init_config
    "nor r0, r0, r0\n"              // NOP just in case compiler puts an ifp? instruction next.
  );

  SEND(&loki_sleep, 11);
  SEND(&receive_init_config, 11);
  SEND_STRUCT(config, sizeof(init_config), 11);

}

inline void loki_init(init_config* config) {
  assert(config->cores > 0);

  config->stack_pointer = (int)malloc(config->stack_size * config->cores)
                                   + (config->stack_size * config->cores);

  // Give each core connections to memory and a stack.
  if (config->cores > 1) {
    uint tile;
    for (tile = 1; tile*CORES_PER_TILE < config->cores; tile++) {
      init_remote_tile(tile, config);
    }

    init_local_tile(config);
  }
  else if (config->config_func != NULL)
    config->config_func();
}

// A wrapper for loki_init which fills in most of the values with sensible
// defaults.
void loki_init_default(const uint cores, const setup_func setup) {

  uint tile = get_tile_id();
  uint core = 0;

  // Instruction channel
  int addr0 = loki_mem_address(CH_IPK_CACHE, tile, 8, core, GROUPSIZE_8, LINESIZE_32);
  int config0 = loki_mem_config(ASSOCIATIVITY_1, LINESIZE_32, CACHE, GROUPSIZE_8);
  SET_CHANNEL_MAP(0, addr0);
//  SEND(config0, 0); // reconfiguration invalidates cache contents

  // Data channel
  int addr1 = loki_mem_address(CH_REGISTER_2, tile, 8, core, GROUPSIZE_8, LINESIZE_32);
  SET_CHANNEL_MAP(1, addr1);

  init_config* config = malloc(sizeof(init_config));
  config->cores = cores;
  config->stack_pointer = 0x400000;
  config->stack_size = 0x12000;
  config->inst_mem = addr0;
  config->data_mem = addr1;
  config->mem_config = config0;
  config->config_func = setup;

  loki_init(config);

}

// Initialise each core between "first" (inclusive) and "last" (exclusive). This
// involves giving each core two connections to memory - one for instructions
// and one for data - and also initialising stack and frame pointers.
// This must be executed right at the beginning of the program, as it needs to
// share the current cache configuration, which is assumed to still be in a
// register.
static inline void init_cores_impl(const int first, const int last) {

  int stack_pointer; // Core 0's default stack pointer
  asm ("addu %0, r8, r0\n" : "=r"(stack_pointer) : : );
  stack_pointer += 0x400 - (stack_pointer & 0x3ff); // Guess at the initial sp
  int bitmask = 0;              // Bitmask of cores to send instructions to

  int current;
  for (current = first; current < last; current++) {
    CONNECT(3,   0,current,7);  // Data input

    // Give each worker a different stack pointer
    stack_pointer -= 0x42000;   // Extra 0x2000 puts stacks in different banks -- or does it?

    asm (
      "addu r0, r20, r0 -> 3\n"       // send icache configuration (assumes it is still in r20)
      "addu r0, r21, r0 -> 3\n"       // send dcache configuration (assumes it is still in r21)
      "addu r0, %0, r0 -> 3\n"        // send core its own stack pointer
      : /* no outputs */
      : "r" (stack_pointer)
      : /* nothing clobbered */
    );

    bitmask |= (1 << current);
  }

  // Set up a multicast connection to all instruction FIFOs
  int fifos = loki_mcast_address(0, bitmask, 0);
  SET_CHANNEL_MAP(2, fifos);

  // Could reduce the size of this by only setting up instruction memory
  // connection, and then fetching a packet which does the rest.
  asm (
    "rmtexecute -> 2\n"             // begin remote execution
    "ifp?cregrdi r11, 1\n"          // get core id, and put into r11
    "ifp?andi r11, r11, 0x7\n"      // get core id, and put into r11
    "ifp?slli r19, r11, 8\n"        // the memory port to connect
    "ifp?addu r17, r7, r19\n"       // complete icache memory address using received value
    "ifp?addu r18, r7, r19\n "      // address of second connection
    "ifp?setchmapi 0, r17\n"
    "ifp?setchmapi 1, r18\n"
    "ifp?addu r8, r7, r0\n"         // receive stack pointer
    "ifp?addu r9, r8, r0\n"         // frame pointer = stack pointer
    "nor r0, r0, r0\n"              // NOP just in case compiler puts an ifp? instruction next.
  );
}

void init_cores(const int num_cores) {
  if (num_cores > 1)
    init_cores_impl(1, num_cores);
}

// Get a core to execute a function. The remote core must already have all of
// the necessary function arguments in the appropriate registers.
void loki_remote_execute(void* func, int core) {
  const int ipk_fifo = loki_core_address(0, core, 0);
  const int data_input = loki_core_address(0, core, 7);
  SET_CHANNEL_MAP(2, ipk_fifo);
  SET_CHANNEL_MAP(3, data_input);

  // Send the function address to the remote core.
  SEND(func, 3);

  // Tell the remote core to fetch the provided function, and sleep when done.
  asm (
    "rmtexecute -> 2\n"
    "ifp?lli r10, %%lo(loki_sleep)\n"
    "ifp?lui r10, %%hi(loki_sleep)\n"
    "ifp?fetch r7\n"
    "nor r0, r0, r0\n"              // NOP just in case compiler puts an ifp? instruction next.
    :::
  );
}

typedef void (*basic_func)(void);
// Execute a function when its address is held in a variable. Any arguments must
// already be in the appropriate registers.
inline void execute(void* func) {
  ((basic_func)(func))();
}

// Signal that all required results have been produced by the parallel execution
// pattern, and that we may now break the cores from their infinite loops.
void end_parallel_section() {

  // Current implementation is to send a token to core 0, input 7.
  // wait_end_parallel_section() must therefore execute on core 0.
  int address = loki_core_address(0, 0, 7);
  SET_CHANNEL_MAP(2, address);
  SEND_TOKEN(2);

}

// Only continue after all cores have executed this function. Tokens from each
// core are collected, then redistributed to show when all have been received.
// TODO: could speedup sync within a tile by using multicast and woche?
void loki_sync(const uint cores) {
  if (cores <= 1)
    return;

  uint core = get_core_id();
  uint tile = get_tile_id();

  // All cores except the final one wait until they receive a token from their
  // neighbour. (A tree structure would be more efficient, but there isn't much
  // difference with so few cores.)
  if ((core < CORES_PER_TILE-1) && (tile*CORES_PER_TILE + core < cores-1))
    RECEIVE_TOKEN(6);

  // All cores except the first one send a token to their other neighbour
  // (after setting up a connection).
  if (core > 0) {
    int address = loki_core_address(tile, core-1, 6);
    SET_CHANNEL_MAP(10, address);
    SEND_TOKEN(10);
    RECEIVE_TOKEN(6);
  }

  // All core 0s then synchronise between tiles using the same process.
  if (core == 0) {
    uint tiles = num_tiles(cores);

    if (tile < tiles-1)
      RECEIVE_TOKEN(5);

    if (tile > 0) {
      int address = loki_core_address(tile-1, 0, 5);
      SET_CHANNEL_MAP(10, address);
      SEND_TOKEN(10);
      RECEIVE_TOKEN(5);
    }

    // All tokens have now been received, so notify all cores.
    if (tile == 0 && tiles > 1) {
      int destination;
      for (destination = 1; destination < tiles; destination++) {
        int address = loki_core_address(destination, 0, 5);
        SET_CHANNEL_MAP(10, address);
        SEND_TOKEN(10);
      }
    }

    // All core 0s need to distribute the token throughout their tiles.
    uint coresThisTile = cores_this_tile(cores, tile);
    if (coresThisTile > 1) {
      int bitmask = all_cores_except_0(coresThisTile);
      int address = loki_mcast_address(tile, bitmask, 6);
      SET_CHANNEL_MAP(10, address);
      SEND_TOKEN(10);
    }
  }
}

void loki_tile_sync(const uint cores) {
  assert(cores <= CORES_PER_TILE);

  if (cores <= 1)
    return;

  uint core = get_core_id();
  uint tile = get_tile_id();

  // All cores except the final one wait until they receive a token from their
  // neighbour. (A tree structure would be more efficient, but there isn't much
  // difference with so few cores.)
  if (core < cores-1)
    RECEIVE_TOKEN(6);

  // All cores except the first one send a token to their other neighbour
  // (after setting up a connection).
  if (core > 0) {
    int address = loki_core_address(tile, core-1, 6);
    SET_CHANNEL_MAP(10, address);
    SEND_TOKEN(10);
    RECEIVE_TOKEN(6);
  }

  // Core 0 notifies all other cores when synchronisation has finished.
  if (core == 0) {
    // All core 0s need to distribute the token throughout their tiles.
    int bitmask = all_cores_except_0(cores);
    int address = loki_mcast_address(tile, bitmask, 6);
    SET_CHANNEL_MAP(10, address);
    SEND_TOKEN(10);
  }
}

// Wait until the end_parallel_section function has been called. This must be
// executed on core 0.
inline void wait_end_parallel_section() {
  RECEIVE_TOKEN(7);
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
  const int tile = get_tile_id();
  const int cores = cores_this_tile(config->cores, tile);

  if (cores > 1) {
    const unsigned int bitmask = all_cores_except_0(cores);
    const int ipk_fifos = loki_mcast_address(tile, bitmask, 0);
    const int data_inputs = loki_mcast_address(tile, bitmask, 7);

    SET_CHANNEL_MAP(10, ipk_fifos);
    SET_CHANNEL_MAP(11, data_inputs);
    SEND(config->data, 11);           // send pointer to function argument(s)
    SEND(&loki_sleep, 11);            // send function pointers
    SEND(config->func, 11);

    // Tell all cores to start executing the loop.
    asm volatile (
      "rmtexecute -> 10\n"            // begin remote execution
      "ifp?addu r13, r7, r0\n"        // receive pointer to arguments
      "ifp?addu r10, r7, r0\n"        // set return address
      "ifp?fetch r7\n"                // fetch function to execute
      "nor r0, r0, r0\n"              // NOP just in case compiler puts an ifp? instruction next.
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
  //RECEIVE(config, 7);
  void* val;
  RECEIVE(val, 7);
  config->cores = (int)val;
  RECEIVE(val, 7);
  config->func = val;
  RECEIVE(val, 7);
  config->data_size = (size_t)val;
  config->data = malloc((size_t)val);
  //RECEIVE(val, 7);
  //config->data = val;

  RECEIVE_STRUCT(config->data, config->data_size, 7);

  distribute_to_local_tile(config);
}

// Start cores on another tile.
void distribute_to_remote_tile(const int tile, const distributed_func* config) {

  // Connect to core 0 in the tile.
  int inst_fifo = loki_core_address(tile, 0, 0);
  int data_input = loki_core_address(tile, 0, 7);
  SET_CHANNEL_MAP(10, inst_fifo);
  SET_CHANNEL_MAP(11, data_input);

  SEND(&loki_sleep, 11);            // send function pointers
  SEND(&receive_config, 11);

  // Tell core 0 to receive configuration struct before setting up all other
  // cores on its tile.
  asm volatile (
    "rmtexecute -> 10\n"            // begin remote execution
    "ifp?addu r10, r7, r0\n"        // set return address
    "ifp?fetch r7\n"                // fetch function to execute
    "nor r0, r0, r0\n"              // NOP just in case compiler puts an ifp? instruction next.
    // No clobbers because this is all executed remotely.
  );

  //SEND(malloc(sizeof(distributed_func)), 11);
  SEND(config->cores, 11);
  SEND(config->func, 11);
  SEND(config->data_size, 11);
  //SEND(malloc(config->data_size), 11);

  SEND_STRUCT(config->data, config->data_size, 11);

}

// The main function to call to execute the same function on many cores.
void loki_execute(const distributed_func* config) {

  if (config->cores > 1) {
    int tile;
    for (tile = 1; tile*CORES_PER_TILE < config->cores; tile++) {
      distribute_to_remote_tile(tile, config);
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
    RECEIVE_TOKEN(6);

  // All cores except the first one send a token to their other neighbour
  // (after setting up a connection).
  if (core > 0) {
    int address = loki_core_address(0, core-1, 6);
    SET_CHANNEL_MAP(2, address);
    SEND_TOKEN(2);
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

    RECEIVE(do_iteration, 7);
    while (do_iteration) {
      func(iter, core-1);

      iter += cores-1;
      RECEIVE(do_iteration, 7);
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
  int address = loki_mcast_address(0, bitmask, 7);
  SET_CHANNEL_MAP(10, address);

  if (config->helper_init != NULL)
    config->helper_init();

  int iterations;
  for (iterations = 0; iterations+simd_cores < total_iterations; iterations += simd_cores) {
    SEND(1, 10); // Would prefer to combine this with the branch instruction.

    // Perform any computation shared by all cores and send results.
    func();
  }

  // Complete any final iterations for which not all cores are needed.
  if (iterations != total_iterations) {
    int iterations_remaining = total_iterations - iterations;

    if (iterations_remaining > 0) {
      int bitmask1 = all_cores_except_0(iterations_remaining + 1); // +1 accounts for helper core
      int address1 = loki_mcast_address(0, bitmask1, 7);
      SET_CHANNEL_MAP(10, address1);

      // Send 1 to cores in bitmask1
      // Perform computation shared by all cores and send results.
      SEND(1, 10);
      func();
    }
  }

  // Send 0 to all cores so they know to stop.
  SET_CHANNEL_MAP(10, address);
  SEND(0, 10);

  // Signal that this core has finished its work. Do we need to tidy() too?
  simd_finished(config, 0);
}

void simd_member(const loop_config* config, const int core) {
  const int tile = get_tile_id();

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
  const int tile = get_tile_id();
  const int cores = (tile*8 < config->cores) ? 8 : (config->cores & 7);
  const unsigned int bitmask = all_cores_except_0(cores);
  const int ipk_fifos = loki_mcast_address(tile, bitmask, 0);
  const int data_inputs = loki_mcast_address(tile, bitmask, 7);

  SET_CHANNEL_MAP(10, ipk_fifos);
  SET_CHANNEL_MAP(11, data_inputs);
  SEND(config, 11);                 // send pointer to configuration info
  SEND(&loki_sleep, 11);            // send function pointers
  SEND(&simd_member, 11);

  // Tell all cores to start executing the loop.
  asm volatile (
    "rmtexecute -> 10\n"            // begin remote execution
    "ifp?addu r13, r7, r0\n"        // receive pointer to configuration info
    "ifp?cregrdi r11, 1\n"          // get core id, and put into r11
    "ifp?andi r11, r11, 0x7\n"      // get core id, and put into r11
    "ifp?addu r14, r11, r0\n"       // put core position in argument-passing register
    "ifp?addu r10, r7, r0\n"        // set return address
    "ifp?fetch r7\n"                // fetch function to execute
    "nor r0, r0, r0\n"              // NOP just in case compiler puts an ifp? instruction next.
    // No clobbers because this is all executed remotely.
  );

}

// Set up a SIMD group on another tile.
void simd_remote_tile(const int tile, const loop_config* config) {

  // Connect to core 0 in the tile.
  int inst_fifo = loki_core_address(tile, 0, 0);
  int data_input = loki_core_address(tile, 0, 7);
  SET_CHANNEL_MAP(10, inst_fifo);
  SET_CHANNEL_MAP(11, data_input);

  SEND(config, 11);                 // send pointer to configuration info
  SEND(&loki_sleep, 11);            // send function pointers
  SEND(&simd_local_tile, 11);

  // Tell core 0 to set up all other cores on its tile.
  // TODO: will the pointer to data work from the other tile?
  asm volatile (
    "rmtexecute -> 10\n"            // begin remote execution
    "ifp?addu r13, r7, r0\n"        // receive pointer to configuration info
    "ifp?addu r10, r7, r0\n"        // set return address
    "ifp?fetch r7\n"                // fetch function to execute
    "nor r0, r0, r0\n"              // NOP just in case compiler puts an ifp? instruction next.
    // No clobbers because this is all executed remotely.
  );

}

// The main function to call to execute the loop in parallel.
void simd_loop(const loop_config* config) {

  if (config->cores > 1) {
    int tile;
    for (tile = 1; tile*8 < config->cores; tile++) {
      simd_remote_tile(tile, config);
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
  SEND(get_core_id(), 2);
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

  int channel; // channel of master core to connect to
  int address = loki_core_address(0, 0, get_core_id() + 2);
  SET_CHANNEL_MAP(2, address);


  // Loop forever, executing the iterations provided. The master will kill the
  // worker with a "nxipk" when all iterations have finished.
  while (1) {
    request_work();

    // Receive the iteration to execute next.
    int iteration;
    RECEIVE(iteration, 7);

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
  const int ipk_fifos = loki_mcast_address(0, bitmask, 0);
  const int data_inputs = loki_mcast_address(0, bitmask, 7);
  SET_CHANNEL_MAP(2, ipk_fifos);
  SET_CHANNEL_MAP(3, data_inputs);

  SEND(config, 3);  // send pointer to configuration info

  asm (
    "rmtexecute -> 2\n"             // begin remote execution
    "ifp?addu r13, r7, r0\n"        // receive pointer to configuration info
    "ifp?cregrdi r11, 1\n"          // get core id, and put into r11
    "ifp?andi r11, r11, 0x7\n"      // get core id, and put into r11
    "ifp?addui r14, r11, -1\n"      // worker ID = core ID - 1 because core 0 is master
    "ifp?lli r10, %lo(loki_sleep)\n" // set return address - sleep when finished
    "ifp?lui r10, %hi(loki_sleep)\n"
    "ifp?lli r24, %lo(worker_thread)\n"
    "ifp?lui r24, %hi(worker_thread)\n"
    "ifp?fetch r24\n"               // fetch the worker's task
    "nor r0, r0, r0\n"              // NOP just in case compiler puts an ifp? instruction next.
  );

  // Issue workers with loop iterations to work on.
  int iter;
  for (iter = 0; iter < config->iterations; iter++) {
    const int worker = receive_any_input();             // wait for any worker to request work
    const int worker_addr = loki_core_address(0, worker, 7);  // compute network address
    SET_CHANNEL_MAP(3, worker_addr);                    // connect
    SEND(iter, 3);                                      // send work
  }

  // Wait for all workers to finish their final tasks, and kill them.
  int finished;
  for (finished = 0; finished < config->cores-1; finished++)
    receive_any_input();             // wait for each worker to request work

  KILL(2);                           // kill all workers

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
    next_addr = loki_core_address(0, stage+1, 6);
  else
    next_addr = loki_core_address(0, 0, 6);

  // Make connection.
  SET_CHANNEL_MAP(2, next_addr);

  // If there is work to do before the pipeline starts, do it now.
  if (config->initialise && config->initialise[stage])
    config->initialise[stage]();

  // Execute loop
  int i;
  for (i=0; i<config->iterations; i++) {
    // If there is a previous core in the pipeline, wait for it to tell us that
    // we can begin work on the next iteration.
    if (have_predecessor)
      RECEIVE_TOKEN(6);

    // Execute this iteration.
    config->stage_func[stage](i);

    // If there is a subsequent core in the pipeline, tell it that it may now
    // begin work on the next iteration.
    if (have_successor)
      SEND_TOKEN(2);
  }

  // Final core tells core 0 when all work has finished.
  if (!have_successor)
    SEND_TOKEN(2);

  // Core 0 only returns when it receives the signal from the final core.
  if (!have_predecessor)
    RECEIVE_TOKEN(6);

  // Release any resources when the pipeline has finished.
  // TODO: can cores other than 0 ever reach this code?
  if (config->tidy && config->tidy[stage])
    config->tidy[stage]();

}

void pipeline_loop(const pipeline_config* config) {

  // Tell all cores to start executing the loop.
  const int bitmask = all_cores_except_0(config->cores);
  const int ipk_fifo = loki_mcast_address(0, bitmask, 0);
  const int data_input = loki_mcast_address(0, bitmask, 7);
  SET_CHANNEL_MAP(2, ipk_fifo);
  SET_CHANNEL_MAP(3, data_input);

  SEND(config, 3);

  asm (
    "rmtexecute -> 2\n"             // begin remote execution
    "ifp?addu r13, r7, r0\n"        // receive pointer to configuration info
    "ifp?cregrdi r11, 1\n"          // get core id, and put into r11
    "ifp?andi r11, r11, 0x7\n"      // get core id, and put into r11
    "ifp?addu r14, r11, r0\n"
    "ifp?lli r10, %lo(loki_sleep)\n"     // set return address - sleep when finished
    "ifp?lui r10, %hi(loki_sleep)\n"     // set return address - sleep when finished
    "ifp?lli r24, %lo(pipeline_stage)\n"
    "ifp?lui r24, %hi(pipeline_stage)\n"
    "ifp?fetch r24\n"               // fetch the pipeline stage's task
    "nor r0, r0, r0\n"              // NOP just in case compiler puts an ifp? instruction next.
  );

  // Also start this core.
  pipeline_stage(config, 0);

}

// Receive an argument from a given register-mapped input, and put it in the
// first argument register.
#define RECEIVE_ARGUMENT(register) {\
  asm volatile (\
    "addu r13, r" #register ", r0\n"\
    ::: "r13" /* no inputs, outputs, or clobbered registers */\
  );\
}

// Send the contents of the first return value register on a given output channel.
#define SEND_RESULT(channel_map_entry) {\
  asm (\
    "addu r0, r11, r0 -> " #channel_map_entry "\n"\
    ::: /* no inputs, outputs, or clobbered registers */\
  );\
}

void dd_pipeline_stage(const dd_pipeline_config* config, const int stage) {

  const int have_predecessor = (stage > 0);
  const int have_successor = (stage < config->cores - 1);

  // Address to connect to. Most cores connect to the next stage in the pipeline
  // so they can tell when it is safe to begin the next loop iteration. The
  // final core, however, tells the first core when all work has finished.
  int next_addr;
  if (have_successor)
    next_addr = loki_core_address(0, stage+1, 6);
  else
    next_addr = loki_core_address(0, 0, 6);

  // Make connection.
  SET_CHANNEL_MAP(2, next_addr);

  // If there is work to do before the pipeline starts, do it now.
  if (config->initialise && config->initialise[stage])
    config->initialise[stage]();

  void* my_func = config->stage_tasks[stage];

  // Stage 0 is in charge of supplying the data to the pipeline, so it should
  // know how much there is and how many iterations to perform.
  if (stage == 0) {
    execute(my_func);
  }
  // All other cores enter an infinite loop, working with the data they receive
  // for as long as it arrives. At some point one core must call the
  // end_parallel_section function.
  else {
    while (1) {
      // Receive an argument from the previous core in the pipeline. (Assuming
      // there is only one argument - more can be passed manually, however.)
//      RECEIVE_ARGUMENT(6);
      int arg;
      RECEIVE(arg, 6);

      if (arg == config->end_of_stream_token) {
        if (have_successor)
          SEND(arg, 2);
        break;
      }

      PUT_IN_REG(arg, 13);  // r13 is the first argument register.

      // Execute this iteration.
      execute(my_func);

      // If there is a subsequent core in the pipeline, send it an argument to
      // work on.
      if (have_successor)
        SEND_RESULT(2);
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
    const int ipk_fifo = loki_mcast_address(0, bitmask, 0);
    const int data_input = loki_mcast_address(0, bitmask, 7);
    SET_CHANNEL_MAP(2, ipk_fifo);
    SET_CHANNEL_MAP(3, data_input);

    SEND(config, 3);

    asm (
      "rmtexecute -> 2\n"             // begin remote execution
      "ifp?addu r13, r7, r0\n"        // receive pointer to configuration info
      "ifp?cregrdi r11, 1\n"          // get core id, and put into r11
      "ifp?andi r11, r11, 0x7\n"      // get core id, and put into r11
      "ifp?addu r14, r11, r0\n"
      "ifp?lli r10, %lo(loki_sleep)\n"     // set return address - sleep when finished
      "ifp?lui r10, %hi(loki_sleep)\n"     // set return address - sleep when finished
      "ifp?lli r24, %lo(dd_pipeline_stage)\n"
      "ifp?lui r24, %hi(dd_pipeline_stage)\n"
      "ifp?fetch r24\n"               // fetch the pipeline stage's task
      "nor r0, r0, r0\n"              // NOP just in case compiler puts an ifp? instruction next.
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
  const int ipk_fifos = loki_mcast_address(0, bitmask, 0);
  SET_CHANNEL_MAP(2, ipk_fifos);
  KILL(2);

  // The dataflow macro stores the address of any tidy-up code which needs to
  // be executed when dataflow has finished. Tell the core to fetch this now.
  asm (
    "rmtexecute -> 2\n"
    "ifp?fetch r24\n"
    "nor r0, r0, r0\n"              // NOP just in case compiler puts an ifp? instruction next.
  );

}


//============================================================================//
// Other
//============================================================================//

#include <stdarg.h>

// Set this function as the return address, and the result of a function will
// be sent on the network, before sending the core to sleep.
// Assumes the result fits into a single register, and that channel map table
// entry 2 has something useful in it.
void send_result() {
  asm (
    "addu.eop r0, r11, r0 -> 2\n"
    :::
  );
}

// Function for the remote core to execute to receive all required arguments.
void spawn_prep() {

  int return_address;
  int func;

  RECEIVE(return_address, 7);
  SET_CHANNEL_MAP(2, return_address);

  RECEIVE(func, 7);

  // Enter an infinite loop, receiving arguments. We will be knocked out of it
  // by the spawner core with a nxipk command, which will take us on to the
  // function.
  asm (
    "fetch %0\n"
    "lli r10, send_result\n" // when function returns, send result
    "addui r22, r0, 13\n" // want to put args into r13 onwards
    "iwtr r22, r7\n"      // receive arg, put in register
    "addui r22, r22, 1\n" // move to next register
    "ibjmp.eop -8\n"      // repeat
    :
    : "r" (func)
    : "r10", "r22"
  );
}

// Execute a function on a remote core, and return its result to a given place.
void loki_spawn(void* func, const int address, const int argc, ...) {
  // Find a free core and connect to it
  int core = 1;

  // Tell the remote core to execute a setup function which will allow it to
  // receive all arguments for the main function.
  loki_remote_execute(&spawn_prep, core);

  // Send all information required to execute the function remotely.
  va_list argp;
  va_start(argp, argc); // argc = last fixed argument
  SEND(address, 3);
  SEND(func, 3);

  int i;
  for (i=0; i<argc; i++) {
    void* arg = va_arg(argp, void*);
    SEND(arg, 3);
  }

  // Have now finished going through arguments - get remote core to execute
  // function. A KILL works for this because the remote core fetched the
  // function before entering an infinite loop, so ending the loop begins the
  // function.
  KILL(2);

  va_end(argp);

}

// Store multiple data values in the extended register file.
//   data:         pointer to the data
//   len:          the number of elements to store
//   reg:          the position in the extended register file to store the first value
//
// Note: three separate versions are needed to force the compiler to use the
// correct load instruction: we do not support unaligned loads, so can't always
// use the integer version.
void scratchpad_block_store_chars(char* data, size_t len, uint reg) {
  uint currReg;
  char* currPtr;
  for (currReg = reg, currPtr = data; currReg < reg+len; currReg++, currPtr++) {
    // TODO: load 4 values at once, then split into registers
    scratchpad_write(currReg, *currPtr);
  }
}
void scratchpad_block_store_shorts(short* data, size_t len, uint reg) {
  uint currReg;
  short* currPtr;
  for (currReg = reg, currPtr = data; currReg < reg+len; currReg++, currPtr++) {
    // TODO: load 2 values at once, then split into registers
    scratchpad_write(currReg, *currPtr);
  }
}
void scratchpad_block_store_ints(int* data, size_t len, uint reg) {
  uint currReg;
  int* currPtr;
  for (currReg = reg, currPtr = data; currReg < reg+len; currReg++, currPtr++) {
    scratchpad_write(currReg, *currPtr);
  }
}
