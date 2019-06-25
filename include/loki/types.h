/*! \file types.h
 * \brief Various constants and types specific to Loki. */

#ifndef LOKI_TYPES_H_
#define LOKI_TYPES_H_

//! Unsigned integer.
typedef unsigned int uint;

//! Boolean values.
#include <stdbool.h>

#include <loki/chip.h>

//! \brief Hack to allow commas to be passed as part of a macro argument, instead of
//! looking like the end of an argument.
//!
//! This is useful when listing registers in the dataflow macros, for example.
#define AND ,

//! Tile ID - a tile on the processor.
typedef unsigned int tile_id_t;
//! Size of \ref tile_id_t in bits.
#define TILE_ID_T_BITS 6
//! Core ID - a core on the processor.
typedef unsigned int core_id_t;

//! Mask to zero out parts of any bitmask which extend beyond what is allowed.
#define SUPPORTED_COREMASK ((1 << CORES_PER_TILE) - 1)

//! Positions of cores within a tile.
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

//! Positions of memory banks within a tile.
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

//! \brief Destinations for multicast communication.
//!
//! This set of values is not exhaustive, but simply provide a useful collection
//! of common destinations. Any bitmask of \ref CORES_PER_TILE bits is valid,
//! with the least significant bit representing core 0.
enum MulticastDestinations {
  MULTICAST_CORE_0 = 0x01 & SUPPORTED_COREMASK, //!< Core 0.
  MULTICAST_CORE_1 = 0x02 & SUPPORTED_COREMASK, //!< Core 1.
  MULTICAST_CORE_2 = 0x04 & SUPPORTED_COREMASK, //!< Core 2.
  MULTICAST_CORE_3 = 0x08 & SUPPORTED_COREMASK, //!< Core 3.
  MULTICAST_CORE_4 = 0x10 & SUPPORTED_COREMASK, //!< Core 4.
  MULTICAST_CORE_5 = 0x20 & SUPPORTED_COREMASK, //!< Core 5.
  MULTICAST_CORE_6 = 0x40 & SUPPORTED_COREMASK, //!< Core 6.
  MULTICAST_CORE_7 = 0x80 & SUPPORTED_COREMASK, //!< Core 7.

  //! No cores mask.
  MULTICAST_CORE_NONE = 0,
  //! Core 0 and core 1.
  MULTICAST_CORE_01 = MULTICAST_CORE_0 | MULTICAST_CORE_1,
  //! Core 2 and core 3.
  MULTICAST_CORE_23 = MULTICAST_CORE_2 | MULTICAST_CORE_3,
  //! Core 4 and core 5.
  MULTICAST_CORE_45 = MULTICAST_CORE_4 | MULTICAST_CORE_5,
  //! Core 6 and core 7.
  MULTICAST_CORE_67 = MULTICAST_CORE_6 | MULTICAST_CORE_7,
  //! Core 0, core 1, core 2 and core 3.
  MULTICAST_CORE_0123 = MULTICAST_CORE_01 | MULTICAST_CORE_23,
  //! Core 4, core 5, core 6 and core 7.
  MULTICAST_CORE_4567 = MULTICAST_CORE_45 | MULTICAST_CORE_67,
  //! All cores mask.
  MULTICAST_CORE_ALL = MULTICAST_CORE_0123 | MULTICAST_CORE_4567,
};

//! Input channels usable in communications with cores.
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
  SCRATCHPAD          = 0, //!< Memory banks behave as scratchpad (no miss handling logic).
  CACHE               = 1  //!< Memory banks behave as cache.
};

//! \deprecated Size of line in cache. No longer supported.
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

//! \deprecated Associativity of each cache bank. No longer supported.
enum MemConfigAssociativity {
  ASSOCIATIVITY_1     = 0, //!< Direct mapped.
  ASSOCIATIVITY_2     = 1, //!< 2 way set associative.
  ASSOCIATIVITY_4     = 2, //!< 4 way set associative.
  ASSOCIATIVITY_8     = 3  //!< 8 way set associative.
};


#endif
