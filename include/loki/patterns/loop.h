/*! \file loop.h
 * \brief Header file for parallel loop execution patterns.
 *
 * Different execution strategies for loops. */

#ifndef LOKI_PATTERN_LOOP_H_
#define LOKI_PATTERN_LOOP_H_

#include <loki/types.h>

//! Function to initilaise the master core.
typedef void (*init_func)(int cores, int iterations, int coreid);
//! Fucntion to iterate.
typedef void (*iteration_func)(int iteration, int coreid);
//! Function to initialise the helper core(s).
typedef void (*helper_func)(void);
//! Function to run on each core after loop.
typedef void (*tidy_func)(int cores, int iterations, int coreid);
//! Function to combine parallel results.
typedef void (*reduce_func)(int cores);

//! Information required to describe the parallel execution of a loop.
typedef struct {
  int                 cores;          //!< Number of cores
  int                 iterations;     //!< Number of iterations
  init_func           initialise;     //!< Function which initialises one core (optional)
  helper_func         helper_init;    //!< Function to initialise helper core (optional)
  iteration_func      iteration;      //!< Function which executes one iteration
  helper_func         helper;         //!< Function to execute data-independent code (optional)
  tidy_func           tidy;           //!< Function run on each core after the loop finishes (optional)
  reduce_func         reduce;         //!< Function which combines all partial results (optional)
} loop_config;

//! \brief Run a loop described by config, with a fixed mapping of iterations to
//! cores.
//!
//! \warning Overwrites channel map table entries 2, 3, replaces and restores 8 and uses `CH_REGISTER_3`.
void simd_loop(const loop_config* config);

//! \brief Run a loop described by config, dynamically allocating iterations to
//! cores as they become available.
//!
//! `config->cores` must be at least 2 and at most 6.
//!
//! \warning Overwrites channel map table entries 2, 3 and uses `CH_REGISTER_3`,
//! `CH_REGISTER_4`, `CH_REGISTER_5`, `CH_REGISTER_6`, `CH_REGISTER_7`.
void worker_farm(const loop_config* config);

#endif
