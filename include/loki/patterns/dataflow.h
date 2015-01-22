/*! \file dataflow.h
 * \brief Header file for the dataflow execution pattern.
 *
 * A group of functions representing a dataflow network. All of the
 * functions except core 0 must make use of the DATAFLOW_PACKET macro.
 *
 * Core 0 is in charge of supplying the pipeline with data, so will contain a
 * loop to do so. All other cores execute a function each time they receive data,
 * and the return value is passed to the next stage of the pipeline.
 *
 * At the end, core 0 must execute the end_parallel_section function when all
 * results have been produced. This can be acheived by having a loop back to
 * core 0 in the network signaling the end of results. */

#ifndef LOKI_PATTERN_DATAFLOW_H_
#define LOKI_PATTERN_DATAFLOW_H_

#include <loki/types.h>

//! Function to run for a dataflow.
typedef void (*dataflow_func)(void);

//! Configuration settings for a dataflow applcation.
typedef struct {
  int                 cores;          //!< Number of cores involved in dataflow.
  dataflow_func*      core_tasks;     //!< A function for each core.
} dataflow_config;

//! Executes the dataflow to completion. Each core will execute its function.
void start_dataflow(const dataflow_config* config);

//! \brief Signal that all results have been produced by the current execution pattern,
//! so it is now possible to break cores out of their infinite loops.
//!
//! Currently used by the dataflow and data-driven pipeline patterns.
void end_parallel_section(void);

//! \brief Define an instruction packet which should be executed repeatedly.
//! \param core integer literal, the core this packet runs on.
//! \param code inline assembler for the looping packet. Must be full instruction packet, ending with .eop and not using fetch.
//! 
//! Must be written in assembly. An ".eop" marker must be present on the last instruction in the
//! packet. Should not modify any registers. Use \ref DATAFLOW_PACKET_2 if temporaries are needed.
#define DATAFLOW_PACKET(core, code) DATAFLOW_PACKET_2(core, code,,,)

//! \brief A dataflow packet which may specify inputs, outputs and clobbered registers.
//! \param core integer literal, the core this packet runs on.
//! \param code inline assembler for the looping packet. Must be full instruction packet, ending with .eop and not using fetch.
//! \param outputs inline assembler output list for all inline assembler.
//! \param inputs inline assembler input list for all inline assembler.
//! \param clobbered inline assembler clobber list for all inline assembler.
//!
//! Must be written in assembly. An ".eop" marker must be present on the last instruction in the
//! packet. May not clobber or modify r24, nor any reserved registers. Use \ref DATAFLOW_PACKET_3
//! if extra setup code is needed.
#define DATAFLOW_PACKET_2(core, code, outputs, inputs, clobbered) asm volatile (\
  "  lli r24, after_dataflow_" #core "\n" /* store the address of the tidy-up code */\
  "  fetchpstr.eop dataflow_core_" #core "\n"\
  "dataflow_core_" #core ":\n"\
  code\
  "after_dataflow_" #core ":\n"\
  :outputs:inputs:clobbered\
)

//! \brief A dataflow packet which may specify inputs, outputs and clobbered registers.
//! \param core integer literal, the core this packet runs on.
//! \param init_code inline assembler to run before the packet loop. Do not end with .eop.
//! \param code inline assembler for the looping packet. Must be full instruction packet, ending with .eop and not using fetch.
//! \param outputs inline assembler output list for all inline assembler.
//! \param inputs inline assembler input list for all inline assembler.
//! \param clobbered inline assembler clobber list for all inline assembler.
//!
//! Must be written in assembly. An ".eop" marker must be present on the last instruction in the
//! packet. May not clobber or modify r24, nor any reserved registers.
#define DATAFLOW_PACKET_3(core, init_code, code, outputs, inputs, clobbered) asm volatile (\
  init_code\
  "  lli r24, after_dataflow_" #core "\n" /* store the address of the tidy-up code */\
  "  fetchpstr.eop dataflow_core_" #core "\n"\
  "dataflow_core_" #core ":\n"\
  code\
  "after_dataflow_" #core ":\n"\
  :outputs:inputs:clobbered\
)

#endif
