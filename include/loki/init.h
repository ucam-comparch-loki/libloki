/*! \file init.h
 * \brief Initialisation routines for lokilib. */

#ifndef LOKI_INIT_H_
#define LOKI_INIT_H_

#include <loki/channels.h>
#include <loki/types.h>

//! Function to be run on each core to do setup.
typedef void (*setup_func)(void);

//! Information required to set up cores for autonomous execution.
typedef struct {
  uint       cores;         //!< Input. Total number of cores to initialise.
  void      *stack_pointer; //!< Input. Stack pointer of core 0. Other stacks are store at stack_pointer - core_id * stack_size.
  size_t     stack_size;    //!< Input. Size of each core's stack (grows down).
  channel_t  inst_mem;      //!< Input. Address/configuration of instruction memory.
  channel_t  data_mem;      //!< Input. Address/configuration of data memory.
  int        mem_config;    //!< Input. Memory configuration (banking, associativity, etc.).
  setup_func config_func;   //!< Input. Function which performs any program-specific setup.
} init_config;

//! \brief Prepare cores for execution.
//!
//! This includes tasks such as creating
//! connections to memory and setting up a private stack.
//! This function must be executed before any parallel code is executed.
void loki_init(init_config* config);

//! \brief Wrapper for loki_init which provides sensible defaults.
void loki_init_default(const uint cores, const setup_func setup);

#endif
