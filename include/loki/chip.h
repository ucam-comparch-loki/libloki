/*! \file chip.h
 * \brief Chip configuration that this library is compiled for.
 *
 * Not all configuration options are supported by this library. For example:
 *  - Numbers of registers and channel ends
 *  - Encodings of channel map table entries, flit metadata, etc.
 *  - Enumeration types (e.g. core and memory bank positions)
 *  - Number of memory banks (static initialisation of memory configurations)
 *  - Some assembly routines make restrictive assumptions
 *    - Cores per tile = banks per tile (\ref loki_memory_reconfigure_setup)
 *    - Directory size = 2 * cores per tile
 *    - Banks per tile = 8
 */
 
#ifndef LOKI_CHIP_H_
#define LOKI_CHIP_H_

//! \brief Number of cores in each tile.
#ifndef CORES_PER_TILE
  #define CORES_PER_TILE 8
#endif

//! \brief Number of memory banks in each tile.
#ifndef BANKS_PER_TILE
  #define BANKS_PER_TILE 8
#endif

//! \brief Number of rows of compute tiles on the chip (excluding I/O halo).
#ifndef COMPUTE_TILE_ROWS
  #define COMPUTE_TILE_ROWS 4
#endif

//! \brief Number of columns of compute tiles on the chip (excluding I/O halo).
#ifndef COMPUTE_TILE_COLUMNS
  #define COMPUTE_TILE_COLUMNS 4
#endif

//! \brief The number of entries in the channel map table, including reserved
//! entries.
#ifndef CHANNEL_MAP_TABLE_SIZE
  #define CHANNEL_MAP_TABLE_SIZE 16
#endif

//! \brief Number of words in each core's scratchpad.
#ifndef SCRATCHPAD_NUM_WORDS
  #define SCRATCHPAD_NUM_WORDS 256
#endif

//! \brief Number of entries stored in each core input buffer.
#ifndef CORE_INPUT_BUFFER_DEPTH
  #define CORE_INPUT_BUFFER_DEPTH 4
#endif

//! \brief Number of instructions stored in each instruction packet FIFO.
#ifndef IPK_FIFO_DEPTH
  #define IPK_FIFO_DEPTH 8
#endif

//! \brief Log base 2 of size of the MHL directory.
#ifndef LOKI_MEMORY_DIRECTORY_SIZE_LOG2
  #define LOKI_MEMORY_DIRECTORY_SIZE_LOG2 4
#endif

//! \brief Size of the MHL directory. Always a power of 2.
#ifndef LOKI_MEMORY_DIRECTORY_SIZE
  #define LOKI_MEMORY_DIRECTORY_SIZE      (1 << LOKI_MEMORY_DIRECTORY_SIZE_LOG2)
#endif
 
 
#endif
 
