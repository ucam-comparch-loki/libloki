// TODO:
//  * merge_and, merge_or, merge_min, etc. generalise to merge(*func)?
//  * parallel_for macro - is this even possible?
//  * bool no_one_finished(), void finished() - use for eurekas, etc
//  * map/reduce

#ifndef LOKILIB_H
#define LOKILIB_H

#include "loki_constants.h"
#include <stddef.h>

typedef unsigned int uint;

//============================================================================//
// Interface
//
//   These should be the only functions/structures needed to modify a program.
//============================================================================//

typedef void (*setup_func)(void);

// Information required to set up cores for autonomous execution.
typedef struct {
  uint       cores;         // Total number of cores to initialise.
  uint       stack_pointer; // Stack pointer of core 0.
  size_t     stack_size;    // Size of each core's stack (grows down).
  int        inst_mem;      // Address/configuration of instruction memory.
  int        data_mem;      // Address/configuration of data memory.
  int        mem_config;    // Memory configuration (banking, associativity, etc.).
  setup_func config_func;   // Function which performs any program-specific setup.
} init_config;

// Prepare cores for execution. This includes tasks such as creating
// connections to memory and setting up a private stack.
// This function must be executed before any parallel code is executed.
void loki_init(init_config* config);

// Wrapper for loki_init which provides sensible defaults.
void loki_init_default(const uint cores, const setup_func setup);

// Deprecated.
// Prepare a given number of cores to execute code later in the program.
// This must be the first thing executed at the start of the program (for now).
void init_cores(const int num_cores);

typedef void (*general_func)(void* data);

// Information required to have all cores execute a particular function.
typedef struct {
  uint         cores;       // Number of cores to execute the function.
  general_func func;        // Function to be executed.
  void*        data;        // Struct to be passed as the function's argument.
  size_t       data_size;   // Size of data in bytes.
} distributed_func;

// Have all cores execute the same function simultaneously.
void loki_execute(const distributed_func* config);

// Wait for all cores between 0 and (cores-1) to reach this point before
// continuing.
// (Warning: quite expensive/slow. Use sparingly.)
void loki_sync(const uint cores);

// Slightly faster synchronisation if all cores are on the same tile.
// Also allows synchronisation which doesn't start at tile 0.
void loki_tile_sync(const uint cores);

// Execute func on another core, and return the result to a given location.
// Note that due to various limitations of parameter passing, the function is
// currently limited to having a maximum of two arguments.
// For the moment, "another core" is always core 1.
void loki_spawn(void* func, const int return_address, const int argc, ...);

// Get a core to execute the instruction packet at the given address. It is
// assumed that all required preparation has already taken place (e.g. storing
// function arguments in the appropriate registers).
void loki_remote_execute(void* address, int core);

// A core will stop work if it executes this function.
void loki_sleep();


//============================================================================//
// Helper functions
//
//   Very simple functions and macros which allow access to some of Loki's
//   unique lower-level features.
//============================================================================//

// Turn the pieces of an address into a single integer.
static inline int loki_core_address(const uint tile, const uint component, const uint channel) {
  return (tile << 20) | (component << 12) | (channel << 8);
}

static inline int loki_mcast_address(const uint tile, const uint bitmask, const uint channel) {
  return (1 << 28) | loki_core_address(tile, bitmask, channel);
}

// Create an encoded memory address. This is to be stored in a core's channel
// map table using SET_CHANNEL_MAP.
static inline int loki_mem_address(const uint returnChannel, const uint tile,
                                   const uint component, const uint channel,
                                   const uint log2GroupSize, const uint log2LineSize) {
  return (returnChannel << 29) | loki_core_address(tile, component, channel)
       | (log2GroupSize << 4) | log2LineSize;
}

// Generate a memory configuration command. Send to a memory group as-is.
static inline int loki_mem_config(const uint log2Assoc, const uint log2LineSize,
                                  const enum MemConfig isCache, const uint log2GroupSize) {
  return (log2Assoc << 20) | (log2LineSize << 16) | (isCache << 8) | (log2GroupSize);
}

// Generate a bitmask with the lowest num_cores bits set to 1. This can then be
// passed to loki_mcast_address above.
static inline uint all_cores(uint num_cores) {
  return (1 << num_cores) - 1;
}

// It is common to have core 0 send data out to all other cores - it does not
// need to send data to itself. num_cores is inclusive of 0.
static inline uint all_cores_except_0(uint num_cores) {
  return all_cores(num_cores) - 1;
}

// Return the minimum number of tiles required to hold the given number of cores.
static inline uint num_tiles(const uint cores) {
  return ((cores - 1)/CORES_PER_TILE) + 1;
}

// Calculate the number of cores that are active on a given tile, given a total
// number of active cores.
static inline uint cores_this_tile(const uint cores, const int tile) {
  return (cores - tile * CORES_PER_TILE > CORES_PER_TILE)
       ? CORES_PER_TILE
       : cores - tile * CORES_PER_TILE;
}

// Return the value in a control register.
#define GET_CONTROL_REGISTER(variable, id) { \
  asm ( \
    "cregrdi %0, " #id "\n" \
    : "=r"(variable) \
    : \
    : \
  );\
}

// Set a control register.
#define SET_CONTROL_REGISTER(variable, id) { \
  asm ( \
    "cregwri %0, " #id "\n" \
    : \
    : "r"(variable) \
    : \
  );\
}

// Return a globally unique idenitifer for this core.
static inline uint get_unique_core_id(void) {
  uint id;
  GET_CONTROL_REGISTER(id, 1);
  return id;
}

// Return the core's position within its tile: an integer between 0 and 7.
static inline uint get_core_id() {
  uint id;
  GET_CONTROL_REGISTER(id, 1);
  return id & 0x7;
}

// Return the ID of the tile the core is in.
static inline uint get_tile_id() {
  uint id;
  GET_CONTROL_REGISTER(id, 1);
  return id >> 4;
}

// Set a channel map entry to the value in the named C variable. entry must
// be an integer literal.
#define SET_CHANNEL_MAP(entry, address) {\
  asm (\
    "setchmapi " #entry ", %0\n"\
    :\
    : "r" (address)\
    :\
  );\
}

// Connect an output to a channel of a remote core or memory. Note that output
// must be an integer literal.
#define CONNECT(output, tile, component, channel) {\
  const int address = loki_core_address(tile, component, channel);\
  SET_CHANNEL_MAP(output, address);\
}

// Get a core in a group to connect to another HELIX-style (wrapping around
// within the group).
//   output: the entry in the channel map table to write (integer literal)
//   offset: core n connects to core (n+offset mod group_size)
//   channel: the input channel of the remote core to connect to
//   group_size: the number of cores in the group. All cores must be contiguous
//               and start at position 0 in the tile
#define HELIX_CONNECT(output, offset, channel, group_size) {\
  int this_core = get_core_id();\
  int next_core = this_core + offset;\
  if (next_core >= group_size)\
    next_core -= group_size;\
  if (next_core < 0)\
    next_core += group_size;\
  int this_tile = get_tile_id();\
  CONNECT(output, this_tile, next_core, channel);\
}

// Set up a connection with memory. It is assumed that the given output channel
// already points to the memory, and this command tells the memory where to
// send data back to.
// If data is sent back to a data input, the core must consume a synchronisation
// message which confirms that all memories are now set up properly.
#define MEMORY_CONNECT(output, return_address) {\
  asm (\
    "addu r0, %0, r0 -> " #output "\n" /* send "table update" command */\
    "addu r0, %1, r0 -> " #output "\n" /* send return address */\
    :: "r" (0x01000000), "r" (return_address) :\
  );\
}

// Ensure that some data is in a particular register. The register must be an
// integer literal.
#define PUT_IN_REG(data, register) {\
  asm volatile (\
    "or r" #register ", %0, r0\n"\
    : : "r" (data) : "r" #register\
  );\
}

// Send the value of the named variable on the given output channel. Note that
// output must be an integer literal, not a variable.
#define SEND(variable, output) {\
  asm (\
    "addu r0, %0, r0 -> " #output "\n"\
    :\
    : "r" ((int)variable)\
    :\
  );\
}

// Receive a value from a particular input and store it in the named variable.
// Note that input must be an integer literal, not a variable.
#define RECEIVE(variable, input) {\
  asm volatile (\
    "addu %0, r" #input ", r0\n"\
    : "=r" (variable)\
    :\
    :\
  );\
}

// Send an entire struct to another core. output must be an integer literal.
// (I can't get this to work properly without hard-coded registers.)
#define SEND_STRUCT(struct_ptr, bytes, output) {\
  asm volatile (\
    "or r12, %0, r0\n"\
    "addu r11, %0, %1\n"\
    "ldw 0(r12) -> 1\n"\
    "addui r12, r12, 4\n"\
    "setlt.p r0, r12, r11\n"\
    "or r0, r2, r0 -> " #output "\n"\
    "ifp?ibjmp -16\n"\
    : \
    : "r" (struct_ptr), "r" (bytes)\
    : "r11", "r12"\
  );\
}

// Receive an entire struct over the network. input must be an integer literal.
#define RECEIVE_STRUCT(struct_ptr, bytes, input) {\
  int end_of_struct;\
  void* struct_ptr_copy;\
  asm volatile (\
    "addu %0, %2, %3\n"\
    "or %1, %2, r0\n"\
    "stw r" #input ", 0(%1) -> 1\n"\
    "addui %1, %1, 4\n"\
    "setlt.p r0, %1, %0\n"\
    "ifp?ibjmp -12\n"\
    : "=r" (end_of_struct), "=r" (struct_ptr_copy)\
    : "r" (struct_ptr), "r" (bytes)\
    :\
  );\
}

// Kill the core at the end of the given channel by sending it a nxipk command.
// This will break it out of infinite loops, and stop it waiting for data, if
// necessary.
#define KILL(output) {\
  asm (\
    "rmtnxipk -> " #output "\n"\
    :::\
  );\
}

// Execute a system call with the given immediate opcode.
#define SYS_CALL(opcode) asm volatile (\
  "  syscall " #opcode "\n"\
)

// The compiler cannot move loads and stores from one side of a barrier to the
// other.
static inline void barrier() {
  asm volatile ("" : : : "memory");
}

// Send a token (0) on a given output channel.
#define SEND_TOKEN(channel_map_entry) {\
  asm (\
    "addu r0, r0, r0 -> " #channel_map_entry "\n"\
    ::: /* no inputs, outputs, or clobbered registers */\
  );\
}

// Receive a token (and discard it) from a given register-mapped input.
#define RECEIVE_TOKEN(register) {\
  asm volatile (\
    "addu r0, r" #register ", r0\n"\
    ::: /* no inputs, outputs, or clobbered registers */\
  );\
}

// Wait for input on any register-mapped input channel, and return the value
// which arrived.
// r18 must be used for irdr in verilog implementation.
static inline int receive_any_input() {
  int data;
  asm volatile (
    "selch r18, 0xFFFFFFFF\n"   // get the channel on which data first arrives
    "irdr %0, r18\n"            // get the data from the channel
    : "=r" (data)
    :
    : "r18"
  );

  return data;
}


//============================================================================//
// Execution patterns
//
//   Data structures and functions used to implement some of the more common
//   execution patterns.
//============================================================================//

typedef void (*init_func)(int cores, int iterations, int coreid);
typedef void (*iteration_func)(int iteration, int coreid);
typedef void (*helper_func)(void);
typedef void (*tidy_func)(int cores, int iterations, int coreid);
typedef void (*reduce_func)(int cores);

typedef void (*pipeline_init_func)(void);
typedef void (*pipeline_func)(int iteration);
typedef void  *dd_pipeline_func;      // Allow any arguments and return values
typedef void (*pipeline_tidy_func)(void);

typedef void (*dataflow_func)(void);

// Information required to describe the parallel execution of a loop.
typedef struct {
  int                 cores;          // Number of cores
  int                 iterations;     // Number of iterations
  init_func           initialise;     // Function which initialises one core (optional)
  helper_func         helper_init;    // Function to initialise helper core (optional)
  iteration_func      iteration;      // Function which executes one iteration
  helper_func         helper;         // Function to execute data-independent code (optional)
  tidy_func           tidy;           // Function run on each core after the loop finishes (optional)
  reduce_func         reduce;         // Function which combines all partial results (optional)
} loop_config;

// Various ways of executing a loop in parallel.
void simd_loop(const loop_config* config);      // All cores execute loop
void worker_farm(const loop_config* config);    // One master, rest of cores are workers


// Information required to describe a pipeline with one core per stage. Cores do
// not communicate data directly; they instead send tokens indicating that all
// necessary work has been done. Each core uses the iteration number to find its
// data. Use dd_pipeline_config for a dataflow-style pipeline.
typedef struct {
  int                 cores;
  int                 iterations;     // Would prefer to be more general than fixed no. iterations?
  pipeline_init_func* initialise;
  pipeline_func*      stage_func;     // A function for each pipeline stage
  pipeline_tidy_func* tidy;
} pipeline_config;

void pipeline_loop(const pipeline_config* config);

// dd = data-driven. This pipeline passes data directly between pipeline stages,
// instead of requiring an intermediate buffer.
// Exactly one core must execute the end_parallel_section function when all
// results have been produced.
typedef struct {
  int                 cores;
  int                 end_of_stream_token;
  pipeline_init_func* initialise;
  dd_pipeline_func*   stage_tasks;    // A function for each pipeline stage
  pipeline_tidy_func* tidy;
} dd_pipeline_config;

void dd_pipeline_loop(const dd_pipeline_config* config);


// A group of functions representing a dataflow network. Most/all of the
// functions will typically make use of the DATAFLOW_PACKET macro.
// Core 0 is in charge of supplying the pipeline with data, so will contain a
// loop to do so. All other cores execute a function each time they receive data,
// and the return value is passed to the next stage of the pipeline.
// Exactly one core must execute the end_parallel_section function when all
// results have been produced.
typedef struct {
  int                 cores;
  dataflow_func*      core_tasks;     // A function for each core
} dataflow_config;

void start_dataflow(const dataflow_config* config);


// Signal that all results have been produced by the current execution pattern,
// so it is now possible to break cores out of their infinite loops.
// Currently used by the dataflow and data-driven pipeline patterns.
void end_parallel_section();

// Define an instruction packet which should be executed repeatedly, in
// assembly. An ".eop" marker must be present on the last instruction in the
// packet.
#define DATAFLOW_PACKET(core, code) DATAFLOW_PACKET_2(core, code,,,)

// A dataflow packet which may specify inputs, outputs and clobbered registers.
#define DATAFLOW_PACKET_2(core, code, outputs, inputs, clobbered) asm volatile (\
  "  lli r24, after_dataflow_" #core "\n" /* store the address of the tidy-up code */\
  "  fetchpstr.eop dataflow_core_" #core "\n"\
  "dataflow_core_" #core ":\n"\
  code\
  "after_dataflow_" #core ":\n"\
  :outputs:inputs:clobbered\
)

// Hack to allow commas to be passed as part of a macro argument, instead of
// looking like the end of an argument.
// This is useful when listing registers in the dataflow macros, for example.
#define AND ,


//============================================================================//
// Scratchpad access
//============================================================================//

// Store multiple data values in the extended register file.
//   data:         pointer to the data
//   len:          the number of elements to store
//   reg:          the position in the extended register file to store the first value
//
// Note that it is possible to cast to a larger datatype if it is acceptable to
// store multiple values in a single register.
void scratchpad_block_store_chars(char* data, size_t len, uint reg);
void scratchpad_block_store_shorts(short* data, size_t len, uint reg);
void scratchpad_block_store_ints(int* data, size_t len, uint reg);

static inline int scratchpad_read(uint reg) {
  int result;
  asm volatile (
    "  scratchrd %0, %1\n"
    : "=r" (result)
    : "r" (reg)
    :
  );
  return result;
}

static inline void scratchpad_write(uint reg, int value) {
  asm volatile (
    "  scratchwr %0, %1\n"
    :
    : "r" (reg), "r" (value)
    :
  );
}


//============================================================================//
// Instrumentation and debug
//============================================================================//

static void start_energy_log()        {SYS_CALL(0x20);}
static void stop_energy_log()         {SYS_CALL(0x21);}
static void start_debug_output()      {SYS_CALL(0x22);}
static void stop_debug_output()       {SYS_CALL(0x23);}
static void start_instruction_trace() {SYS_CALL(0x24);}
static void stop_instruction_trace()  {SYS_CALL(0x25);}

static inline uint current_cycle() {
  uint val;
  asm (
    "syscall 0x30\n"
    "or %0, r12, r0\n" // Assuming less than 2^32 cycles, otherwise need r11 too
    : "=r" (val)
    :
    : "r11", "r12"
  );
  return val;
}
	
#endif /* LOKILIB_H */
