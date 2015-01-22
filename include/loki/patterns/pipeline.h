/*! \file pipeline.h
 * \brief Header file for the pipeline execution pattern.
 *
 * In the pipeline pattern, cores do not communicate data directly;
 * they instead send tokens indicating that all necessary work has been done.
 * Each core uses the iteration number to find its  data.
 *
 * In the data driven (dd) pipeline pattern, data passes data directly between
 * pipeline stages, instead of requiring an intermediate buffer. Exactly one
 * core must execute the end_parallel_section function when all results have
 * been produced.
 */

#ifndef LOKI_PATTERN_PIPELINE_H_
#define LOKI_PATTERN_PIPELINE_H_

#include <loki/types.h>

//! Function to run once to on each core to initialise a stage.
typedef void (*pipeline_init_func)(void);
//! Function to execute a stage.
typedef void (*pipeline_func)(int iteration);
//! Function to execute a data driven stage (arbitrary inputs an outputs.).
typedef void  *dd_pipeline_func;      // Allow any arguments and return values
//! Function to tidy after each stage.
typedef void (*pipeline_tidy_func)(void);

//! Information required to describe a pipeline with one core per stage.
typedef struct {
  int                 cores;          //!< Number of cores.
  int                 iterations;     //!< Number of iterations to run.
  pipeline_init_func* initialise;     //!< Array of initialisation functions for each stage (optional).
  pipeline_func*      stage_func;     //!< A function for each pipeline stage
  pipeline_tidy_func* tidy;           //!< Array of tidy up functions for each stage (optional).
} pipeline_config;

//! Start a pipeline pattern with the given config.
void pipeline_loop(const pipeline_config* config);

//! Information required to describe a data driven pipeline with one core per stage.
typedef struct {
  int                 cores;               //!< Number of cores.
  int                 end_of_stream_token; //!< Special return value which will signal the end of the pipeline.
  pipeline_init_func* initialise;          //!< Array of initialisation functions for each stage (optional).
  dd_pipeline_func*   stage_tasks;         //!< A function for each pipeline stage
  pipeline_tidy_func* tidy;                //!< Array of tidy up functions for each stage (optional).
} dd_pipeline_config;

//! Start a data driven pipeline pattern with the given config.
void dd_pipeline_loop(const dd_pipeline_config* config);

#include <loki/patterns/dataflow.h> // For end_parallel_section.

#endif
