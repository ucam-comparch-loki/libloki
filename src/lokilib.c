#include <loki/lokilib.h>
#include <loki/sendconfig.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

//============================================================================//
// Shared
//
//   Various helper functions used by the parallel implementations.
//============================================================================//

static void init_run_config(setup_func func, unsigned int cores) {
  func();
  // Ensure all cores (across all tiles) are done with the config func before returning.
  // This ensures no race conditions exist with configuration functions.
  loki_sync(cores);
}

// More flexible core initialisation. To be executed by core 0 of any tile.
static void init_local_tile(const init_config* config) {
  uint tile = get_tile_id();
  uint cores = cores_this_tile(config->cores, tile, tile_id(1, 1));

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
  channel_t data_mcast = loki_mcast_address(all_cores_except_0(cores), 2, false);

  set_channel_map(2, inst_mcast);
  set_channel_map(3, data_mcast);

  loki_send(3, inst_mem);
  loki_send(3, data_mem);
  loki_send(3, stack_pointer);
  loki_send(3, stack_size);

  asm volatile (
    "fetchr 0f\n"
    "rmtexecute -> 2\n"         // begin remote execution
    "cregrdi r11, 1\n"          // get core id, and put into r11
    "andi r11, r11, 0x7\n"      // get core id, and put into r11
    "slli r23, r11, 2\n"        // the memory port to connect to
    "addu r17, r2, r23\n"       // combine port with received inst_mem
    "addu r18, r2, r23\n"       // combine port with received data_mem
    "setchmapi 0, r17\n"
    "setchmapi 1, r18\n"
    "or r8, r2, r0\n"           // receive stack pointer
    "mullw r14, r2, r11\n"      // multiply core ID by stack size
    "subu r8, r8, r14\n"
    "or.eop r9, r8, r0\n"       // frame pointer = stack pointer
    "0:\n"
  );

  if (config->config_func != NULL) {
    loki_send(3, (int)&loki_sleep); // return address
    loki_send(3, (int)config->config_func); // arg1
    loki_send(3, (int)config->cores); // arg2
    loki_send(3, (int)&init_run_config); // function

    asm volatile (
      "fetchr 0f\n"
      "rmtexecute -> 2\n"       // begin remote execution
      "or r10, r2, r0\n"        // set return address
      "or r13, r2, r0\n"        // set arg1
      "or r14, r2, r0\n"        // set arg2
      "fetch.eop r2\n"          // fetch function to perform further init
      "0:\n"
    );

    // run the same code as other cores.
    init_run_config(config->config_func, config->cores);
  } else {
    // Since there is no config func, all cores are ready at this point. Just need a tile to tile sync.
    loki_sync_tiles(num_tiles(config->cores));
  }
}

static inline void init_remote_tile(const tile_id_t tile, const init_config* config) {

  // Send initial configuration.
  int data_input = loki_core_address(tile, 0, 3, INFINITE_CREDIT_COUNT);
  set_channel_map(2, data_input);
  loki_send(2, config->inst_mem);
  loki_send(2, config->data_mem);
  loki_send(2, (int)config->stack_pointer - tile2int(tile)*CORES_PER_TILE*config->stack_size);
  
  // Send some instructions to execute.
  int inst_fifo = loki_core_address(tile, 0, 0, INFINITE_CREDIT_COUNT);
  set_channel_map(2, inst_fifo);

  asm volatile (
    "fetchr 0f\n"
    "rmtexecute -> 2\n "        // begin remote execution
    "setchmapi 0, r3\n"         // instruction channel
    "setchmapi 1, r3\n"         // data channel
    "nor r0, r0, r0\n"          // nop after setchmap before channel use
    "or r8, r3, r0\n"           // receive stack pointer
    "or r9, r8, r0\n"           // frame pointer = stack pointer
    "lli r10, %lo(loki_sleep)\n"
    "lui.eop r10, %hi(loki_sleep)\n"// return address = sleep
    "0:\n"
  );

  // Have the remote core initialise the rest of its tile. (config has already
  // been flushed in loki_init.)
  loki_remote_execute(tile, 0, &init_local_tile, (void*)config, sizeof(init_config));

}

void loki_init(init_config* config) {
  assert(config->cores > 0);

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
    loki_channel_flush_data(1, config, sizeof(init_config));
    unsigned int tile;
    for (tile = 1; tile*CORES_PER_TILE < config->cores; tile++) {
      init_remote_tile(int2tile(tile), config);
    }

    init_local_tile(config);
  }
  else if (config->config_func != NULL)
    config->config_func();
}

// Initialise a particular set of tiles.
void loki_init_tiles(int num_tiles, tile_id_t* tile_ids, init_config* config) {
  bool init_self = false;

  for (unsigned int tile = 0; tile < num_tiles; tile++) {
    if (tile_ids[tile] == get_tile_id()) {
      init_self = true;
      continue;
    }
    else
      init_remote_tile(tile_ids[tile], config);
  }
  
  if (init_self)
    init_local_tile(config);
    
  // FIXME: these initialisation functions include a synchronisation, but don't
  // know which tiles to synchronise.
}

// A wrapper for loki_init which fills in most of the values with sensible
// defaults.
void loki_init_default(const uint cores, const setup_func setup) {
  init_config *config = loki_malloc(sizeof(init_config));
  config->cores = cores;
  config->stack_pointer = 0;
  config->stack_size = 0x12000;
  config->inst_mem = 0;
  config->data_mem = 0;
  config->config_func = setup;

  loki_init(config);
  
  loki_free(config);

}

//============================================================================//
// Reconfiguration
//
//   Allow safe dynamic reconfiguration of the memory system.
//============================================================================//

// Give each core a connection to a distinct memory bank all replies to the
// master core.
static void loki_memory_reconfigure_setup_task(enum Cores core) {
  channel_t channel =
    loki_cache_address(
      (enum Memories)get_core_id()
    , core
    , get_core_id() & 1 ? CH_REGISTER_6 : CH_REGISTER_4
    , GROUPSIZE_1
    );
  set_channel_map(3, channel);
}

// Have all cores execute loki_memory_reconfigure_setup_task.
static inline void loki_memory_reconfigure_setup(void) {
  channel_t address;

  address = loki_mcast_address(all_cores_except_current(8), CH_IPK_FIFO, false);
  set_channel_map(2, address);
  address = loki_mcast_address(all_cores_except_current(8), CH_REGISTER_3, false);
  set_channel_map(3, address);

  assert(BANKS_PER_TILE == CORES_PER_TILE);
  asm volatile (
    "fetchr 0f\n"
    "or r0, %2, r0 -> 3\n"
    "rmtexecute -> 2\n"
    "lli r10, %%lo(%0)\n"
    "lui r10, %%hi(%0)\n"
    "lli r11, %%lo(%1)\n"
    "lui r11, %%hi(%1)\n"
    "or r13, r3, r0\n"
    "fetch.eop r11\n0:\n"
  :
  : "i"(loki_sleep), "i"(loki_memory_reconfigure_setup_task), "r"(get_core_id())
  );

  loki_memory_reconfigure_setup_task(get_core_id());
}

// Send the new directory entries to the appropriate core.
static inline void loki_memory_reconfigure_send_directory_entries(
  loki_memory_directory_configuration_t const value
) {
  assert(LOKI_MEMORY_DIRECTORY_SIZE == CORES_PER_TILE * 2);

  for (int i = 0; i < LOKI_MEMORY_DIRECTORY_SIZE; i += 2) {
    channel_t address =
      loki_mcast_address(single_core_bitmask(i/2), CH_REGISTER_5, false);
    set_channel_map(4, address);

    loki_send(4, (i + 0) << value.mask_index);
    loki_send(4, loki_memory_directory_entry_to_int(value.entries[i + 0]));
    loki_send(4, (i + 1) << value.mask_index);
    loki_send(4, loki_memory_directory_entry_to_int(value.entries[i + 1]));
  }
}

// Send the new instruction and data channels to each core.
static inline void loki_memory_reconfigure_send_channels(
  loki_memory_cache_configuration_t const value
) {
  assert(BANKS_PER_TILE == 8);

  for (int i = 0; i < CORES_PER_TILE; i++) {
    channel_t address =
      loki_mcast_address(single_core_bitmask(i), CH_REGISTER_3, false);
    set_channel_map(4, address);

    // Make a bitmask of banks used by this core for icache and dcache.
    int dbitmask = 0;
    int ibitmask = 0;
    for (int j = 0; j < BANKS_PER_TILE; j++) {
      if (value.banks[j].dcache & single_core_bitmask(i))
        dbitmask |= 0x101 << j;
      if (value.banks[j].icache & single_core_bitmask(i))
        ibitmask |= 0x101 << j;
    }

    channel_t dmem, imem;

    // Default to discard addresses.
    dmem = loki_mcast_address(0, CH_REGISTER_2, false);
    imem = loki_mcast_address(0, CH_IPK_CACHE, false);

    enum MemConfigGroupSize group_size;
    int gs;

    // Detect the group size and start for dcache.
    for (gs = 8, group_size = GROUPSIZE_8; gs > 0; gs /= 2, group_size--) {
      for (int j = 0; j < BANKS_PER_TILE; j++) {
        if (((dbitmask >> j) & ((1 << gs) - 1)) == ((1 << gs) - 1)) {
          dmem = loki_mem_address(
              j, i, CH_REGISTER_2, group_size
            , value.dcache_skip_l1 & single_core_bitmask(i)
            , value.dcache_skip_l2 & single_core_bitmask(i)
            , false
            );
          gs = 1;
          break;
        }
      }
    }

    // Detect the group size and start for icache.
    for (gs = 8, group_size = GROUPSIZE_8; gs > 0; gs /= 2, group_size--) {
      for (int j = 0; j < BANKS_PER_TILE; j++) {
        if (((ibitmask >> j) & ((1 << gs) - 1)) == ((1 << gs) - 1)) {
          imem = loki_mem_address(
              j, i, CH_IPK_CACHE, group_size
            , value.icache_skip_l1 & single_core_bitmask(i)
            , value.icache_skip_l2 & single_core_bitmask(i)
            , false
            );
          gs = 1;
          break;
        }
      }
    }

    loki_send(4, dmem);
    loki_send(4, imem);
  }
}

// Change all cores caches simultaneously.
void loki_memory_cache_reconfigure(
  loki_memory_cache_configuration_t const value
) {
  channel_t address;

  loki_memory_reconfigure_setup();
  loki_memory_reconfigure_send_channels(value);

  // Setup instruction broadcast for later.
  address = loki_mcast_address(all_cores(8), CH_IPK_FIFO, false);
  set_channel_map(2, address);

  // Have all other cores execute loki_sleep when done.
  address = loki_mcast_address(all_cores_except_current(8), CH_REGISTER_3, false);
  set_channel_map(4, address);
  loki_send(4, (int)&loki_sleep);
  address = loki_mcast_address(single_core_bitmask(get_core_id()), CH_REGISTER_3, false);
  set_channel_map(4, address);

  int t0;

  // Send all the necessary instructions to all cores including this one.
  // That way no IFetch will be triggered while this code is running.
  asm volatile (
    "fetchr 0f\n"
    "rmtexecute -> 2\n"
    "sendconfig r0, %1 -> 3\n" // Flush all lines.
    "cregrdi %0, 1\n"
    "andi.p r0, %0, 1\n"
    "if!p?sendconfig r0, %2 -> 3\n" // Ping.
    "ifp?sendconfig  r0, %3 -> 3\n" // Ping.
    "nor.eop r0, r0, r0\n"
    "0:\n" // Since all other cores idle, at this point all banks are flusing.
    "addui r0, r1, 0f - 0b -> 4\n"
    "or r0, r4, r6\n" // Wait.
    "or r0, r4, r6\n" // Wait.
    "or r0, r4, r6\n" // Wait.
    "or r0, r4, r6\n" // Wait.
    "rmtexecute -> 2\n"
    "setchmapi 1, r3\n" // Data channel.
    "setchmapi 0, r3\n" // Instruction channel.
    "nor r0, r0, r0\n"
    "sendconfig r0, %4 -> 3\n" // Invalidate all lines.
    "fetch.eop r3\n"
    "0:\n"
  : "=&r"(t0)
  : "n" (SC_FLUSH_ALL_LINES),
    "n" (SC_RETURN_TO_R4 | SC_L1_SCRATCHPAD | SC_LOAD_WORD),
    "n" (SC_RETURN_TO_R6 | SC_L1_SCRATCHPAD | SC_LOAD_WORD),
    "n" (SC_INVALIDATE_ALL_LINES)
  : "memory"
  );
}

// Change the directory atomically.
void loki_memory_directory_reconfigure(
  loki_memory_directory_configuration_t const value
) {
  channel_t address;

  loki_memory_reconfigure_setup();
  loki_memory_reconfigure_send_directory_entries(value);

  // Setup instruction broadcast for later.
  address = loki_mcast_address(all_cores(8), CH_IPK_FIFO, false);
  set_channel_map(2, address);

  // Have all other cores execute loki_sleep when done.
  address = loki_mcast_address(all_cores_except_current(8), CH_REGISTER_3, false);
  set_channel_map(4, address);
  loki_send(4, (int)&loki_sleep);
  address = loki_mcast_address(single_core_bitmask(get_core_id()), CH_REGISTER_3, false);
  set_channel_map(4, address);

  int t0;

  // Send all the necessary instructions to all cores including this one.
  //
  // The gist of this code is that each cores sends a flush all lines to the
  // corresponding bank. To determine if this has finished, they then send a
  // load. Core 0 receives all of these loads, which allows it to act as global
  // syncrohnisation. Core 0 then reconfigures the directory mask and then each
  // core changes to directory entries.
  //
  // Once the reconfiguration starts, we must prevent all IFetch, which is done
  // by ensuring the reconfiguration will be an IPK hit on core 0, and will be
  // executed form the FIFO on other cores.
  asm volatile (
    "fetchr 0f\n"
    // Broadcast flush all lines to each core (including this one).
    "rmtexecute -> 2\n"
    "sendconfig r0, %2 -> 3\n" // Flush all lines.
    "cregrdi %0, 1\n"
    "andi.p r0, %0, 1\n"
    // Even banks send result to r4, odd to r6. This avoids filling the buffer
    // and therefore deadlock.
    "if!p?sendconfig r0, %3 -> 3\n" // Ping.
    "ifp?sendconfig  r0, %4 -> 3\n" // Ping.
    // Need predicate to be true on all cores so ifp? broadcast does execute.
    "seteq.p.eop r0, r0, r0\n"
    "0:\n"
    // Need predicate to be false on core 0 so packet doesn't execute first time
    // round.
    "fetchr 0f\n"
    "setne.p.eop r0, r0, r0\n"
    // We execute this packet twice to ensure an IPK hit, thus preventing all
    // fetch during reconfigure sequence. The first time round, pred is false
    // throughout.
    "0:\n"
    // The packet ends with fetch r3; this send tells core 0 where to go. The
    // other cores all go to loki_sleep (already in r3).
    "if!p?addui r0, r1, 1f - 0b -> 4\n"
    "ifp?addui r0, r1, 0f - 0b -> 4\n"
    "ifp?or r0, r4, r6\n" // Wait.
    "ifp?or r0, r4, r6\n" // Wait.
    "ifp?or r0, r4, r6\n" // Wait.
    "ifp?or r0, r4, r6\n" // Wait.
    "ifp?sendconfig r0, %6 -> 3\n" // Directory mask.
    "ifp?sendconfig %1, %7 -> 3\n" // Directory mask.
    // First time round we don't want to broadcast the instructions.
    "ifp?rmtexecute -> 2\n"
    "ifp?sendconfig r0, %5 -> 3\n" // Invalidate all lines.
    "ifp?sendconfig r5, %8 -> 3\n" // Directory entry.
    "ifp?sendconfig r5, %7 -> 3\n" // Directory entry.
    "ifp?sendconfig r5, %8 -> 3\n" // Directory entry.
    "ifp?sendconfig r5, %7 -> 3\n" // Directory entry.
    "fetch.eop r3\n"
    "1:\n"
    "fetchr 0b\n"
    "seteq.p.eop r0, r0, r0\n"
    "0:\n"
  : "=&r"(t0)
  : "r"(value.mask_index), "n" (SC_FLUSH_ALL_LINES),
    "n" (SC_RETURN_TO_R4 | SC_L1_SCRATCHPAD | SC_LOAD_WORD),
    "n" (SC_RETURN_TO_R6 | SC_L1_SCRATCHPAD | SC_LOAD_WORD),
    "n" (SC_INVALIDATE_ALL_LINES), "n" (SC_UPDATE_DIRECTORY_MASK),
    "n" (SC_PAYLOAD_EOP), "n" (SC_UPDATE_DIRECTORY_ENTRY)
  : "memory"
  );
}

// Change both caches and directory simultaneously atomically.
void loki_memory_tile_reconfigure(
  loki_memory_cache_configuration_t const cache
, loki_memory_directory_configuration_t const directory
) {
  channel_t address;

  loki_memory_reconfigure_setup();
  loki_memory_reconfigure_send_directory_entries(directory);
  loki_memory_reconfigure_send_channels(cache);

  // Setup instruction broadcast for later.
  address = loki_mcast_address(all_cores(8), CH_IPK_FIFO, false);
  set_channel_map(2, address);

  // Have all other cores execute loki_sleep when done.
  address = loki_mcast_address(all_cores_except_current(8), CH_REGISTER_3, false);
  set_channel_map(4, address);
  loki_send(4, (int)&loki_sleep);
  address = loki_mcast_address(single_core_bitmask(get_core_id()), CH_REGISTER_3, false);
  set_channel_map(4, address);

  int t0;

  // Send all the necessary instructions to all cores including this one.
  //
  // The gist of this code is that each cores sends a flush all lines to the
  // corresponding bank. To determine if this has finished, they then send a
  // load. Core 0 receives all of these loads, which allows it to act as global
  // syncrohnisation. Core 0 then reconfigures the directory mask and then each
  // core changes two directory entries. The data channel can be changed
  // relatively early (as it's not used by this code) but the instruction
  // channel must be changed at the last minute once all instruction fetch is
  // done.
  //
  // Once the reconfiguration starts, we must prevent all IFetch, which is done
  // by ensuring the reconfiguration will be an IPK hit on core 0, and will be
  // executed form the FIFO on other cores.
  asm volatile (
    "fetchr 0f\n"
    // Broadcast flush all lines to each core (including this one).
    "rmtexecute -> 2\n"
    "sendconfig r0, %2 -> 3\n" // Flush all lines.
    "cregrdi %0, 1\n"
    "andi.p r0, %0, 1\n"
    // Even banks send result to r4, odd to r6. This avoids filling the buffer
    // and therefore deadlock.
    "if!p?sendconfig r0, %3 -> 3\n" // Ping.
    "ifp?sendconfig  r0, %4 -> 3\n" // Ping.
    "setchmapi 1, r3\n" // Data channel.
    // Need predicate to be true on all cores so ifp? broadcast does execute.
    "seteq.p.eop r0, r0, r0\n"
    "0:\n"
    // Need predicate to be false on core 0 so packet doesn't execute first time
    // round.
    "fetchr 0f\n"
    "setne.p.eop r0, r0, r0\n"
    // We execute this packet twice to ensure an IPK hit, thus preventing all
    // fetch during reconfigure sequence. The first time round, pred is false
    // throughout.
    "0:\n"
    // The packet ends with fetch r3; this send tells core 0 where to go. The
    // other cores all go to loki_sleep (already in r3).
    "if!p?addui r0, r1, 1f - 0b -> 4\n"
    "ifp?addui r0, r1, 0f - 0b -> 4\n"
    "ifp?or r0, r4, r6\n" // Wait.
    "ifp?or r0, r4, r6\n" // Wait.
    "ifp?or r0, r4, r6\n" // Wait.
    "ifp?or r0, r4, r6\n" // Wait.
    "ifp?sendconfig r0, %6 -> 3\n" // Directory mask.
    "ifp?sendconfig %1, %7 -> 3\n" // Directory mask.
    // First time round we don't want to broadcast the instructions.
    "ifp?rmtexecute -> 2\n"
    // First time round, we must resend r3 so it's in the right place.
    "if!p?or r0, r3, r0 -> 4\n"
    // Second time round, we set the instruction channel.
    "ifp?setchmapi 0, r3\n" // Instruction channel.
    "ifp?sendconfig r0, %5 -> 3\n" // Invalidate all lines.
    "ifp?sendconfig r5, %8 -> 3\n" // Directory entry.
    "ifp?sendconfig r5, %7 -> 3\n" // Directory entry.
    "ifp?sendconfig r5, %8 -> 3\n" // Directory entry.
    "ifp?sendconfig r5, %7 -> 3\n" // Directory entry.
    "fetch.eop r3\n"
    "1:\n"
    "fetchr 0b\n"
    "seteq.p.eop r0, r0, r0\n"
    "0:\n"
  : "=&r"(t0)
  : "r"(directory.mask_index), "n" (SC_FLUSH_ALL_LINES),
    "n" (SC_RETURN_TO_R4 | SC_L1_SCRATCHPAD | SC_LOAD_WORD),
    "n" (SC_RETURN_TO_R6 | SC_L1_SCRATCHPAD | SC_LOAD_WORD),
    "n" (SC_INVALIDATE_ALL_LINES), "n" (SC_UPDATE_DIRECTORY_MASK),
    "n" (SC_PAYLOAD_EOP), "n" (SC_UPDATE_DIRECTORY_ENTRY)
  : "memory"
  );
}

// Signal that all required results have been produced by the parallel execution
// pattern, and that we may now break the cores from their infinite loops.
void end_parallel_section() {

  // Current implementation is to send a token to core 0, input 3.
  // wait_end_parallel_section() must therefore execute on core 0.
  int address = loki_mcast_address(single_core_bitmask(0), CH_REGISTER_3, false);
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
    loki_receive_token(CH_REGISTER_7);

  // All tiles except the first one send a token to their other neighbour
  // (after setting up a connection).
  if (tile > 0) {
    int address = loki_core_address(int2tile(tile-1), 0, CH_REGISTER_7, INFINITE_CREDIT_COUNT);
    set_channel_map(2, address);
    loki_send_token(2);
    loki_receive_token(CH_REGISTER_7);
  } else {
    // All tokens have now been received, so notify all tiles.
    assert(tile == 0);
    int destination;
    for (destination = 1; destination < tiles; destination++) {
      int address = loki_core_address(int2tile(destination), 0, CH_REGISTER_7, INFINITE_CREDIT_COUNT);
      set_channel_map(2, address);
      loki_send_token(2);
    }
  }
}

// Only continue after all cores have executed this function. Tokens from each
// core are collected, then redistributed to show when all have been received.
// TODO: could speedup sync within a tile by using multicast until buffers fill?
void loki_sync_ex(const uint cores, const tile_id_t first_tile) {
  if (cores <= 1)
    return;
    
  assert(cores <= 128);

  uint core = get_core_id();
  tile_id_t tile = get_tile_id();
  uint coresThisTile = cores_this_tile(cores, tile, first_tile);

  // All cores except the final one wait until they receive a token from their
  // neighbour. (A tree structure would be more efficient, but there isn't much
  // difference with so few cores.)
  if (core < coresThisTile-1)
    loki_receive_token(CH_REGISTER_3);

  // All cores except the first one send a token to their other neighbour
  // (after setting up a connection).
  if (core > 0) {
    int address = loki_mcast_address(single_core_bitmask(core-1), CH_REGISTER_3, false);
    set_channel_map(2, address);
    loki_send_token(2);
    
    // Receive token from core 0, telling us that synchronisation has finished.
    loki_receive_token(CH_REGISTER_3);
  } else {
    // All core 0s then synchronise between tiles using the same process.
    assert(core == 0);

    loki_sync_tiles(num_tiles(cores));

    // All core 0s need to distribute the token throughout their tiles.
    if (coresThisTile > 1) {
      int bitmask = all_cores_except_0(coresThisTile);
      int address = loki_mcast_address(bitmask, CH_REGISTER_3, false);
      set_channel_map(2, address);
      loki_send_token(2);
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
    loki_receive_token(CH_REGISTER_3);

  // All cores except the first one send a token to their other neighbour
  // (after setting up a connection).
  if (core > 0) {
    int address = loki_mcast_address(single_core_bitmask(core-1), CH_REGISTER_3, false);
    set_channel_map(2, address);
    loki_send_token(2);
    loki_receive_token(CH_REGISTER_3);
  }

  // Core 0 notifies all other cores when synchronisation has finished.
  if (core == 0) {
    // All core 0s need to distribute the token throughout their tiles.
    int bitmask = all_cores_except_0(cores);
    int address = loki_mcast_address(bitmask, CH_REGISTER_3, false);
    set_channel_map(2, address);
    loki_send_token(2);
  }
}

// Wait until the end_parallel_section function has been called. This must be
// executed on core 0.
static inline void wait_end_parallel_section() {
  loki_receive_token(CH_REGISTER_3);
}

//============================================================================//
// Distributed execution
//
//   All cores execute the same function, which may itself choose different
//   roles for different cores.
//============================================================================//

typedef struct distributed_func_internal_ {
  distributed_func const *config;
  tile_id_t first_tile;
} distributed_func_internal;

// Start cores on the current tile. This must be executed by core 0.
static void distribute_to_local_tile(const distributed_func_internal *internal) {
  const distributed_func *config = internal->config;
  
  // Invalidate all data and refetch it to ensure it is up-to-date if we are on
  // a different tile.
  if (get_tile_id() != internal->first_tile) {
    loki_channel_invalidate_data(1, config, sizeof(distributed_func));
    loki_channel_invalidate_data(1, config->data, config->data_size);
  }

  // Make multicast connections to all other members of the SIMD group.
  const tile_id_t tile = get_tile_id();
  const int cores = cores_this_tile(config->cores, tile, internal->first_tile);

  if (cores > 1) {
    const unsigned int bitmask = all_cores_except_0(cores);
    const channel_t ipk_fifos = loki_mcast_address(bitmask, 0, false);
    const channel_t data_inputs = loki_mcast_address(bitmask, 3, false);

    set_channel_map(2, data_inputs);
    loki_send(2, (int)config->data);      // send pointer to function argument(s)
    loki_send(2, (int)&loki_sleep);       // send function pointers
    loki_send(2, (int)config->func);
    
    // Tell all cores to start executing the loop.
    set_channel_map(2, ipk_fifos);
    asm volatile (
      "fetchr 0f\n"
      "rmtexecute -> 2\n"         // begin remote execution
      "addu r13, r3, r0\n"        // receive pointer to arguments
      "addu r10, r3, r0\n"        // set return address
      "fetch.eop r3\n"            // fetch function to execute
      "0:\n"
      // No clobbers because this is all executed remotely.
    );
  }

  // Now that all the other cores are going, this core can start on its share of
  // the work.
  config->func(config->data);

}

// Start cores on another tile.
static void distribute_to_remote_tile(tile_id_t tile, distributed_func_internal const *internal) {
  // Assume that all data has already been flushed.
  
  // Have core 0 of the remote tile share the data with all other cores there.
  loki_remote_execute(tile, 0, &distribute_to_local_tile, (void*)internal, sizeof(distributed_func_internal));
}

// The main function to call to execute the same function on many cores.
void loki_execute(const distributed_func* config) {
  // Multiple tiles: malloc data, and share via main memory.
  if (config->cores > CORES_PER_TILE) {      
    distributed_func_internal* internal =
        loki_malloc(sizeof(distributed_func_internal));
    internal->config = config;
    internal->first_tile = get_tile_id();

    // Flush the configuration data so it is accessible to the remote tiles.
    loki_channel_flush_data(1, internal, sizeof(distributed_func_internal));
    loki_channel_flush_data(1, config, sizeof(distributed_func));
    loki_channel_flush_data(1, config->data, config->data_size);

    int tile;
    int thisTile = tile2int(get_tile_id());
    for (tile = 1; tile*CORES_PER_TILE < config->cores; tile++) {
      distribute_to_remote_tile(int2tile(tile + thisTile), internal);
    }

    distribute_to_local_tile(internal);

    // TODO: free(internal) when we know all cores have finished with it.
  }
  
  // Single tile: allocate on stack and share locally. malloc relies on global
  // variables, and is unsafe.
  else if (config->cores > 1) {
    distributed_func_internal internal;
    internal.config = config;
    internal.first_tile = get_tile_id();
    
    distribute_to_local_tile(&internal);
    // TODO: Would like a sync here to make sure all cores have finished with
    // data before deallocating it.
  }
  
  // Single core: execute function directly.
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

struct loop_config_internal {
  loop_config const * config;
  tile_id_t first_tile;
};

// Signal that this core has finished its share of the SIMD loop. This is done
// in such a way that when the control core (the one that started the SIMD stuff)
// receives the signal, it knows that all cores have finished and that it is
// safe to continue through the program.
// TODO: it may be worth combining this step with the reduction stage - e.g.
// instead of sending a token, send this core's partial result.
static inline void simd_finished(const loop_config* config, int core) {

  // All cores except the final one wait until they receive a token from their
  // neighbour. (A tree structure would be more efficient, but there isn't much
  // difference with so few cores.)
  if (core < config->cores - 1)
    loki_receive_token(3);

  // All cores except the first one send a token to their other neighbour
  // (after setting up a connection).
  if (core > 0) {
    int address = loki_mcast_address(single_core_bitmask(core-1), 3, false);
    set_channel_map(2, address);
    loki_send_token(2);
  }

}

// The code that a single core in the SIMD array executes. Note that the
// iterations are currently striped across the cores - this avoids needing a
// division to find out which cores execute which iterations, but may reduce
// locality.
static void worker_core(const loop_config* config, int core) {

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

    do_iteration = loki_receive(3);
    while (do_iteration) {
      func(iter, core-1);

      iter += cores-1;
      do_iteration = loki_receive(3);
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
void loki_sleep(void) {
  asm (
    "or.eop r0, r0, r0\n"
  );
  __builtin_unreachable();
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
  int address = loki_mcast_address(bitmask, 3, false);
  channel_t c8 = channel_map_swap(8, address);

  if (config->helper_init != NULL)
    config->helper_init();

  int iterations;
  for (iterations = 0; iterations+simd_cores < total_iterations; iterations += simd_cores) {
    loki_send(8, 1); // Would prefer to combine this with the branch instruction.

    // Perform any computation shared by all cores and send results.
    func();
  }

  // Complete any final iterations for which not all cores are needed.
  if (iterations != total_iterations) {
    int iterations_remaining = total_iterations - iterations;

    if (iterations_remaining > 0) {
      int bitmask1 = all_cores_except_0(iterations_remaining + 1); // +1 accounts for helper core
      int address1 = loki_mcast_address(bitmask1, 3, false);
      set_channel_map(8, address1);

      // Send 1 to cores in bitmask1
      // Perform computation shared by all cores and send results.
      loki_send(8, 1);
      func();
    }
  }

  // Send 0 to all cores so they know to stop.
  set_channel_map(8, address);
  loki_send(8, 0);
  channel_map_restore(8, c8);

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
static void simd_local_tile(const struct loop_config_internal* internal) {
  loop_config const *config = internal->config;

  // Make multicast connections to all other members of the SIMD group.
  const tile_id_t tile = get_tile_id();
  const int cores = cores_this_tile(config->cores, tile, internal->first_tile);
  const unsigned int bitmask = all_cores_except_0(cores);
  const channel_t ipk_fifos = loki_mcast_address(bitmask, 0, false);
  const channel_t data_inputs = loki_mcast_address(bitmask, 3, false);

  set_channel_map(2, ipk_fifos);
  set_channel_map(3, data_inputs);
  loki_send(3, (int)config);            // send pointer to configuration info
  loki_send(3, (int)&loki_sleep);       // send function pointers
  loki_send(3, (int)&simd_member);

  // Tell all cores to start executing the loop.
  asm volatile (
    "fetchr 0f\n"
    "rmtexecute -> 2\n"         // begin remote execution
    "addu r13, r3, r0\n"        // receive pointer to configuration info
    "cregrdi r11, 1\n"          // get core id, and put into r11
    "andi r11, r11, 0x7\n"      // get core id, and put into r11
    "addu r14, r11, r0\n"       // put core position in argument-passing register
    "addu r10, r3, r0\n"        // set return address
    "fetch.eop r3\n"            // fetch function to execute
    "0:\n"
    // No clobbers because this is all executed remotely.
  );

}

// Set up a SIMD group on another tile.
static void simd_remote_tile(const tile_id_t tile, const struct loop_config_internal *internal) {

  // Connect to core 0 in the tile.
  channel_t inst_fifo = loki_core_address(tile, 0, 0, INFINITE_CREDIT_COUNT);
  channel_t data_input = loki_core_address(tile, 0, 3, INFINITE_CREDIT_COUNT);
  set_channel_map(2, inst_fifo);
  set_channel_map(3, data_input);

  loki_send(3, (int)internal);               // send pointer to configuration info
  loki_send(3, (int)&loki_sleep);            // send function pointers
  loki_send(3, (int)&simd_local_tile);

  // Tell core 0 to set up all other cores on its tile.
  // TODO: will the pointer to data work from the other tile?
  asm volatile (
    "fetchr 0f\n"
    "rmtexecute -> 2\n"         // begin remote execution
    "addu r13, r3, r0\n"        // receive pointer to configuration info
    "addu r10, r3, r0\n"        // set return address
    "fetch.eop r3\n"            // fetch function to execute
    "0:\n"
    // No clobbers because this is all executed remotely.
  );

}

// The main function to call to execute the loop in parallel.
void simd_loop(const loop_config* config) {
  struct loop_config_internal internal = {
      .config = config
    , .first_tile = get_tile_id()
  };

  if (config->cores > 1) {
    int tile;
    for (tile = 1; tile < num_tiles(config->cores); tile++) {
      simd_remote_tile(int2tile(tile), &internal);
    }

    simd_local_tile(&internal);
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
  channel_t c8 = channel_map_swap(8, address);

  // Loop forever, executing the iterations provided. The master will kill the
  // worker by sending -1 as the iteration.
  while (1) {
    loki_send(8, get_core_id());

    // Receive the iteration to execute next.
    int iteration;
    iteration = loki_receive(3);

    if (iteration == -1) break; // end of work signal
    // Execute the loop iteration.
    config->iteration(iteration, worker);
  }
  channel_map_restore(8, c8);
}

// Distribute all loop iterations across the workers, and wait for them all to
// complete.
void worker_farm(const loop_config* config) {
  assert(config->cores > 1);
  assert(config->cores <= 6); // Currently limited by number of master's input ports

  // Make multicast connections to all workers.
  unsigned int bitmask = all_cores_except_0(config->cores);
  const channel_t ipk_fifos = loki_mcast_address(bitmask, 0, false);
  const channel_t data_inputs = loki_mcast_address(bitmask, 3, false);
  set_channel_map(2, ipk_fifos);
  set_channel_map(3, data_inputs);

  loki_send(3, (int)config);  // send pointer to configuration info

  asm (
    "fetchr 0f\n"
    "rmtexecute -> 2\n"             // begin remote execution
    "addu r13, r3, r0\n"            // receive pointer to configuration info
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
    const int worker_addr = loki_mcast_address(single_core_bitmask(worker), 3, false);
    set_channel_map(3, worker_addr);                    // connect
    loki_send(3, iter);                                      // send work
  }

  // Wait for all workers to finish their final tasks, and kill them.
  int finished;
  for (finished = 0; finished < config->cores-1; finished++) {
    const int worker = receive_any_input();             // wait for any worker to request work
    const int worker_addr = loki_mcast_address(single_core_bitmask(worker), 3, false);
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
    next_addr = loki_mcast_address(single_core_bitmask(stage+1), 3, false);
  else
    next_addr = loki_mcast_address(single_core_bitmask(0), 3, false);

  // Make connection.
  channel_t c8 = channel_map_swap(8, next_addr);

  // If there is work to do before the pipeline starts, do it now.
  if (config->initialise && config->initialise[stage])
    config->initialise[stage]();

  // Execute loop
  int i;
  for (i=0; i<config->iterations; i++) {
    // If there is a previous core in the pipeline, wait for it to tell us that
    // we can begin work on the next iteration.
    if (have_predecessor)
      loki_receive_token(3);

    // Execute this iteration.
    config->stage_func[stage](i);

    // If there is a subsequent core in the pipeline, tell it that it may now
    // begin work on the next iteration.
    if (have_successor)
      loki_send_token(3);
  }

  // Final core tells core 0 when all work has finished.
  if (!have_successor)
    loki_send_token(8);

  // Core 0 only returns when it receives the signal from the final core.
  if (!have_predecessor)
    loki_receive_token(3);

  channel_map_restore(8, c8);

  // Release any resources when the pipeline has finished.
  // TODO: can cores other than 0 ever reach this code?
  if (config->tidy && config->tidy[stage])
    config->tidy[stage]();

}

void pipeline_loop(const pipeline_config* config) {

  // Tell all cores to start executing the loop.
  const int bitmask = all_cores_except_0(config->cores);
  const channel_t ipk_fifo = loki_mcast_address(bitmask, 0, false);
  const channel_t data_input = loki_mcast_address(bitmask, 3, false);
  set_channel_map(2, ipk_fifo);
  set_channel_map(3, data_input);

  loki_send(3, (int)config);

  asm (
    "fetchr 0f\n"
    "rmtexecute -> 2\n"             // begin remote execution
    "addu r13, r3, r0\n"            // receive pointer to configuration info
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
    next_addr = loki_mcast_address(single_core_bitmask(stage+1), 3, false);
  else
    next_addr = loki_mcast_address(single_core_bitmask(0), 3, false);

  // Make connection.
  channel_t c8 = channel_map_swap(8, next_addr);

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
          loki_send(8, result);
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
      arg = loki_receive(3);

      if (arg == config->end_of_stream_token) {
        if (have_successor)
          loki_send(8, arg);
        break;
      }

      // Execute this iteration.
      int result = my_func(arg);

      // If there is a subsequent core in the pipeline, send it an argument to
      // work on.
      if (have_successor)
        loki_send(8, result);
    }
  }

  channel_map_restore(8, c8);

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
    const channel_t data_input = loki_mcast_address(bitmask, 3, false);
    set_channel_map(2, ipk_fifo);
    set_channel_map(3, data_input);

    loki_send(3, (int)config);

    asm (
      "fetchr 0f\n"
      "rmtexecute -> 2\n"           // begin remote execution
      "addu r13, r3, r0\n"          // receive pointer to configuration info
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
    loki_remote_execute(get_tile_id(), core, config->core_tasks[core], NULL, 0);

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
// Memory
//============================================================================//

extern void* __heap_ptr;

void loki_free(void* ptr) {
  // The standard free does what we want here.
  free(ptr);
}

// Round val up to the nearest multiple of the cache line size.
static inline int loki_round_up_cache_line(int val) {
  if (val & 0x1F)
    return (val & ~0x1F) + 32;  // Clear offset, then add a whole line.
  else
    return val;
}

void* loki_malloc(size_t size) {
  if (tile2int(get_tile_id()) != 0) {
    printf("Warning: calling malloc on tile %d is unsafe!\n", 
           tile2int(get_tile_id()));
  }
             
  // Round the heap pointer up to the nearest cache line boundary.
  __heap_ptr = (void*)loki_round_up_cache_line((int)__heap_ptr);
  
  // Round up size so we allocate a whole number of cache lines.
  size_t newSize = loki_round_up_cache_line(size);
  
  // Normal malloc.
  return malloc(newSize);  
}

void* loki_calloc (size_t num, size_t size) {
  void* ptr = loki_malloc(num*size);
  memset(ptr, 0, num*size);
  return ptr;
}

void* loki_realloc (void* ptr, size_t size) {
  if (size == 0) {
    loki_free(ptr);
    return NULL;
  }
  else {
    // Would ideally like to avoid allocation if the requested size is less than
    // the current size, but we don't have the current size.
    void* newPtr = loki_malloc(size);
    
    if (ptr != NULL) {
      // This is likely to copy too much, but again, we don't know how much data
      // is in the current allocation.
      memcpy(newPtr, ptr, size);
      loki_free(ptr);
    }
    
    return newPtr;
  }
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

  return_address = loki_receive(3);

  func = (void *)loki_receive(3);

  int argc = loki_receive(3);
  int i;
  assert(argc <= 5);
  for (i = 0; i < argc; i++) {
    args[i] = loki_receive(3);
  }

  // Calling func like this assumes nothing bad happens if the function doesn't use the later arguments.
  // This is a reasonable assumption for 5 arguments, but beyond that it's a bit dodgy.
  int returnValue = func(args[0], args[1], args[2], args[3], args[4]);

  set_channel_map(2, return_address);
  loki_send(2, returnValue);
}

// Execute a function on a remote core, and return its result to a given place.
// Deprecated: please use loki_spawn which connects to a particular core, and
// pass the return address as an additional argument.
void loki_spawn(void* func, const channel_t address, const int argc, ...) {
  // Find a free core and connect to it
  int core = 1;

  // r13-r17
  assert(argc <= 5);

  // Tell the remote core to execute a setup function which will allow it to
  // receive all arguments for the main function.
  loki_remote_execute(get_tile_id(), core, &spawn_prep, NULL, 0);

  const channel_t data_input = loki_core_address_ex(core, 3);
  set_channel_map(3, data_input);

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

void refresh_args_before_call(void *func(void*), void* args, size_t arg_size) {
  // Force the arguments to be fetched again, so they are up to date.
  loki_channel_invalidate_data(1, args, arg_size);
  func(args);
}

// Execute a function on a given core.
void loki_remote_execute(tile_id_t tile, core_id_t core, void* func, void* args, size_t arg_size) {
  // Execute locally. We don't want to reset the return pointer like we do in
  // the remote cases. Apologies for the horrible cast!
  if (tile == get_tile_id() && core == get_core_id()) {
    ((void (*)(void *))func)(args);
  }
  
  // Execute on same tile. Memory is coherent here, so just point the core to
  // the necessary data.
  else if (tile == get_tile_id()) {
    // Send all necessary information to a data channel.
    const channel_t data_input = loki_core_address(tile, core, 3, INFINITE_CREDIT_COUNT);
    set_channel_map(2, data_input);
    loki_send(2, (int)args);
    loki_send(2, (int)func);
    
    // Tell remote core which code to execute.
    const channel_t inst_input = loki_core_address(tile, core, 0, INFINITE_CREDIT_COUNT);
    set_channel_map(2, inst_input);  
    asm volatile (
      "fetchr 0f\n"
      "rmtexecute -> 2\n"           // The rest of this packet executes remotely
      "lli r10, %lo(loki_sleep)\n"
      "lui r10, %hi(loki_sleep)\n"  // Set return address
      "or r13, r3, r0\n"            // Put args pointer into first argument register
      "fetch.eop r3\n"              // Start executing func
      "0:\n"
    );
  }
  
  // Execute on a remote tile. Memory is incoherent, so we must force the
  // arguments to be refreshed in memory
  else {
    // Send all necessary information to a data channel.
    const channel_t data_input = loki_core_address(tile, core, 3, INFINITE_CREDIT_COUNT);
    set_channel_map(2, data_input);
    loki_send(2, (int)func);
    loki_send(2, (int)args);
    loki_send(2, (int)arg_size);
    
    // Tell remote core which code to execute.
    const channel_t inst_input = loki_core_address(tile, core, 0, INFINITE_CREDIT_COUNT);
    set_channel_map(2, inst_input);  
    asm volatile (
      "fetchr 0f\n"
      "rmtexecute -> 2\n"           // The rest of this packet executes remotely
      "lli r10, %lo(loki_sleep)\n"
      "lui r10, %hi(loki_sleep)\n"  // Set return address
      "or r13, r3, r0\n"            // Receive func
      "or r14, r3, r0\n"            // Receive args
      "or r15, r3, r0\n"            // Receive arg_size
      "lli r16, %lo(refresh_args_before_call)\n"
      "lui r16, %hi(refresh_args_before_call)\n"
      "fetch.eop r16\n"             // Start executing refresh_args_before_call
      "0:\n"
    );
  }
}
