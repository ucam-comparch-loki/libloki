/*! \file lokilib.h
 * \brief This header includes all other lokilib headers. */
// TODO:
//  * merge_and, merge_or, merge_min, etc. generalise to merge(*func)?
//  * parallel_for macro - is this even possible?
//  * bool no_one_finished(), void finished() - use for eurekas, etc
//  * map/reduce

#ifndef LOKILIB_H
#define LOKILIB_H

#include <stddef.h>

#include <loki/types.h>

//============================================================================//
// Interface
//
//   These should be the only functions/structures needed to modify a program.
//============================================================================//

#include <loki/init.h>
#include <loki/spawn.h>

//============================================================================//
// Helper functions
//
//   Very simple functions and macros which allow access to some of Loki's
//   unique lower-level features.
//============================================================================//

#include <loki/barrier.h>
#include <loki/channels.h>
#include <loki/control_registers.h>
#include <loki/memory.h>
#include <loki/syscall.h>

//============================================================================//
// Execution patterns
//
//   Data structures and functions used to implement some of the more common
//   execution patterns.
//============================================================================//

#include <loki/patterns/dataflow.h>
#include <loki/patterns/loop.h>
#include <loki/patterns/pipeline.h>

//============================================================================//
// Scratchpad access
//============================================================================//

#include <loki/scratchpad.h>

//============================================================================//
// Instrumentation and debug
//============================================================================//

#include <loki/lokisim.h>

#include <loki/deprecated.h>
	
#endif /* LOKILIB_H */
