/*! \file types.h
 * \brief Various constants and types specific to Loki. */

#ifndef LOKI_TYPES_H_
#define LOKI_TYPES_H_

//! Unsigned integer.
typedef unsigned int uint;

//! Boolean values.
#include <stdbool.h>

//! \brief Hack to allow commas to be passed as part of a macro argument, instead of
//! looking like the end of an argument.
//!
//! This is useful when listing registers in the dataflow macros, for example.
#define AND ,

//! Number of cores in each tile.
#define CORES_PER_TILE 8

//! Number of rows of compute tiles on the chip (excluding I/O halo).
#define COMPUTE_TILE_ROWS 4

//! Number of columns of compute tiles on the chip (excluding I/O halo).
#define COMPUTE_TILE_COLUMNS 4

//! Tile ID - a tile on the processor.
typedef unsigned int tile_id_t;

//! Components of a tile.
enum Cores {
  COMPONENT_CORE_0, //!< Core 0.
  COMPONENT_CORE_1, //!< Core 1.
  COMPONENT_CORE_2, //!< Core 2.
  COMPONENT_CORE_3, //!< Core 3.
  COMPONENT_CORE_4, //!< Core 4.
  COMPONENT_CORE_5, //!< Core 5.
  COMPONENT_CORE_6, //!< Core 6.
  COMPONENT_CORE_7  //!< Core 7.
};

enum Memories {
  COMPONENT_BANK_0, //!< Cache Bank 0.
  COMPONENT_BANK_1, //!< Cache Bank 1.
  COMPONENT_BANK_2, //!< Cache Bank 2.
  COMPONENT_BANK_3, //!< Cache Bank 3.
  COMPONENT_BANK_4, //!< Cache Bank 4.
  COMPONENT_BANK_5, //!< Cache Bank 5.
  COMPONENT_BANK_6, //!< Cache Bank 6.
  COMPONENT_BANK_7  //!< Cache Bank 7.
};

//! Destinations for a muticast (flags).
enum MulticastDestinations {
  MULTICAST_CORE_0 = 0x01, //!< Core 0.
  MULTICAST_CORE_1 = 0x02, //!< Core 1.
  MULTICAST_CORE_2 = 0x04, //!< Core 2.
  MULTICAST_CORE_3 = 0x08, //!< Core 3.
  MULTICAST_CORE_4 = 0x10, //!< Core 4.
  MULTICAST_CORE_5 = 0x20, //!< Core 5.
  MULTICAST_CORE_6 = 0x40, //!< Core 6.
  MULTICAST_CORE_7 = 0x80  //!< Core 7.
};

//! Input channels usable in communications.
enum Channels {
  CH_IPK_FIFO, //!< Instruction FIFO.
  CH_IPK_CACHE, //!< Instruction packet cache. Only usable in memory configurations.
  CH_REGISTER_2, //!< r2 channel. The C compiler uses this for all memory requests.
  CH_REGISTER_3, //!< r3 channel.
  CH_REGISTER_4, //!< r4 channel.
  CH_REGISTER_5, //!< r5 channel.
  CH_REGISTER_6, //!< r6 channel.
  CH_REGISTER_7  //!< r7 channel.
};

//! System calls. Used in simulators to communicate with environment.
enum SystemCalls {
  SYS_EXIT            = 0x01,   //!< Terminate the program
  SYS_OPEN            = 0x02,   //!< Open a file
  SYS_CLOSE           = 0x03,   //!< Close a file
  SYS_READ            = 0x04,   //!< Read from a file
  SYS_WRITE           = 0x05,   //!< Write to a file
  SYS_SEEK            = 0x06,   //!< Seek within a file

  SYS_TILE_ID         = 0x10,   //!< \deprecated Unique ID of this tile. Use \ref get_tile_id instead.
  SYS_POSITION        = 0x11,   //!< \deprecated Position within this tile. Use \ref get_core_id instead.

  /* These are all lokisim specific. */
  SYS_ENERGY_LOG_ON   = 0x20,   //!< Start recording energy-consuming events
  SYS_ENERGY_LOG_OFF  = 0x21,   //!< Stop recording energy-consuming events
  SYS_DEBUG_ON        = 0x22,   //!< Print lots of information to stdout
  SYS_DEBUG_OFF       = 0x23,   //!< Stop printing debug information
  SYS_INST_TRACE_ON   = 0x24,   //!< Print address of every instruction executed
  SYS_INST_TRACE_OFF  = 0x25,   //!< Stop printing instruction addresses

  SYS_CURRENT_CYCLE   = 0x30    //!< \deprecated Get the current cycle number
};

//! Cache mode.
enum MemConfigType {
  SCRATCHPAD          = 0, //!< Cache behaves as scratchpad (no miss handling logic).
  CACHE               = 1  //!< Cahce behaves as cache.
};

//! Size of line in cache.
enum MemConfigLineSize {
  LINESIZE_1          = 0, //!< 1 word.
  LINESIZE_2          = 1, //!< 2 words.
  LINESIZE_4          = 2, //!< 4 words.
  LINESIZE_8          = 3, //!< 8 words.
  LINESIZE_16         = 4, //!< 16 words.
  LINESIZE_32         = 5, //!< 32 words.
  LINESIZE_64         = 6, //!< 64 words.
};

//! Number of banks acting as a group.
enum MemConfigGroupSize {
  GROUPSIZE_1         = 0, //!< 1 bank.
  GROUPSIZE_2         = 1, //!< 2 banks.
  GROUPSIZE_4         = 2, //!< 4 banks.
  GROUPSIZE_8         = 3, //!< 8 banks.
  GROUPSIZE_16        = 4, //!< 16 banks.
  GROUPSIZE_32        = 5, //!< 32 banks.
  GROUPSIZE_64        = 6  //!< 64 banks.
};

//! Associativity of each cache bank.
enum MemConfigAssociativity {
  ASSOCIATIVITY_1     = 0, //!< Direct mapped.
  ASSOCIATIVITY_2     = 1, //!< 2 way set associative.
  ASSOCIATIVITY_4     = 2, //!< 4 way set associative.
  ASSOCIATIVITY_8     = 3  //!< 8 way set associative.
};

//! Possible execution environments.
enum Environments {
  ENV_NONE,     //!< The variable has not yet been set.
  ENV_LOKISIM,  //!< lokisim simulator.
  ENV_FPGA,     //!< FPGA.
  ENV_VCS,      //!< Synopsys' VCS verilog simulator.
  ENV_CSIM,     //!< CSim behavioural simulator.
  ENV_VERILATOR //!< Verilator verilog simulator.
};

//! \brief The execution environment of the current program.
//!
//! In an ideal world, it shouldn't matter what hardware or simulator is
//! running the application; they should all be the same architecture. However,
//! the nature of the project is that they all have subtle differences and
//! implemented different features, so some sensible way of testing the support
//! is needed. Unfortunately, most simulators crash on an unsupported feature,
//! so feature testing must be done in advance.
//!
//! This variable not valid until a loki_init call.
extern enum Environments environment;
//! \brief The version of the execution environment (where such a value is meaningful).
//!
//! An environment specific header will define values for this variable if it
//! is used. If not, it will be 0.
//!
//! This variable not valid until a loki_init call.
extern int environment_version;

#endif
