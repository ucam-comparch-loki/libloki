/*! \file spawn.h
 * \brief Remote execution routines for lokilib. */

#ifndef LOKI_SPAWN_H_
#define LOKI_SPAWN_H_

#include <loki/types.h>

//! A function to operate on arbitrary data.
typedef void (*general_func)(void* data);

//! Information required to have all cores execute a particular function.
typedef struct {
  uint         cores;       //!< Number of cores to execute the function.
  general_func func;        //!< Function to be executed.
  void*        data;        //!< Struct to be passed as the function's argument.
  size_t       data_size;   //!< Size of data in bytes.
} distributed_func;

//! Have all cores execute the same function simultaneously.
void loki_execute(const distributed_func* config);

//! \brief Wait for all cores between 0 and (cores-1) to reach this point before
//! continuing.
//!
//! (Warning: quite expensive/slow. Use sparingly.)
void loki_sync(const uint cores);

//! \brief Wait for all cores between 0 and (cores-1) on a tile to reach this point before
//! continuing.
//!
//! Slightly faster synchronisation if all cores are on the same tile.
//! Also allows synchronisation which doesn't start at tile 0.
void loki_tile_sync(const uint cores);

//! \brief Execute func on another core, and return the result to a given location.
//!
//! Note that due to various limitations of parameter passing, the function is
//! currently limited to having a maximum of two arguments.
//! For the moment, "another core" is always core 1.
void loki_spawn(void* func, const int return_address, const int argc, ...);

//! \brief Get a core to execute the instruction packet at the given address.
//!
//! It is
//! assumed that all required preparation has already taken place (e.g. storing
//! function arguments in the appropriate registers).
void loki_remote_execute(void* address, int core);

//! A core will stop work if it executes this function.
void loki_sleep();

#endif
