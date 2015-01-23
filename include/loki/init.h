/*! \file init.h
 * \brief Initialisation routines for lokilib. */

#ifndef LOKI_INIT_H_
#define LOKI_INIT_H_

#include <loki/channels.h>
#include <loki/types.h>

//! Function to be run on each core to do setup.
typedef void (*setup_func)(void);

//! \brief Information required to set up cores for autonomous execution.
//!
//! See \ref loki_init for details.
typedef struct {
  uint       cores;         //!< Input. Total number of cores to initialise.
  void      *stack_pointer; //!< Input. Stack pointer of core 0 (optional).
  size_t     stack_size;    //!< Input. Size of each core's stack (grows down).
  channel_t  inst_mem;      //!< Input. Address/configuration of instruction memory.
  channel_t  data_mem;      //!< Input. Address/configuration of data memory.
  int        mem_config;    //!< Input. Memory configuration (banking, associativity, etc.).
  setup_func config_func;   //!< Input. Function which performs any program-specific setup (optional).
} init_config;

//! \brief Prepare cores for execution.
//!
//! This includes tasks such as creating connections to memory and setting up a
//! private stack. The specified number of cores are initialised across tiles
//! starting at 0. Exactly `num_tiles(config->cores)` tiles will be started.
//!
//! Each core's stack pointer will be set by
//! `config->stack_pointer - core_id * stack_size`. If `config->stack_pointer` is
//! `NULL`, the `config->stack_pointer` will be set to the executable's default.
//!
//! Channel 0 on each core will
//! be set to `config->inst_mem` (with corrected tile and core values).
//! Similarly, Channel 1 on each core will be set to `config->data_mem.`
//!
//! The memory configuration command (see \ref loki_mem_config) `config->mem_config`
//! will be sent by the first core on each tile to the memory system except on tile 0
//! where this is usually done by the bootloader.
//!
//! The function `config->config_func` is run on each core. This is optional, set to
//! `NULL` to skip.
//!
//! This function must be executed before any other function in this library.
//! It may overwrite any channel map table entry on any core, and generate network
//! traffic.
void loki_init(init_config* config);

//! \brief Wrapper for loki_init which provides sensible defaults.
//! \param cores value to set `config->cores` to.
//! \param setup value to set `config->config_func` to.
//!
//! `config->stack_size` is set to to 0x12000.
//!
//! `config->inst_mem` and `config->data_mem` are set to the bootloader's defaults.
//!
//! `config->mem_config` is set to `loki_mem_config(ASSOCIATIVITY_1, LINESIZE_32, CACHE, GROUPSIZE_8)`.
//!
//! This function must be executed before any other function in this library.
//! It may overwrite any channel map table entry on any core, and generate network
//! traffic.
void loki_init_default(const uint cores, const setup_func setup);

#endif
