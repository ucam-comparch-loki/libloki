/*! \file spawn.h
 * \brief Remote execution routines for lokilib. */

#ifndef LOKI_SPAWN_H_
#define LOKI_SPAWN_H_

#include <loki/types.h>

//! A function to operate on arbitrary data.
typedef void (*general_func)(const void* data);

//! Information required to have all cores execute a particular function.
typedef struct {
  uint         cores;       //!< Number of cores to execute the function.
  general_func func;        //!< Function to be executed.
  const void  *data;        //!< Struct to be passed as the function's argument.
  size_t       data_size;   //!< Size of data in bytes.
} distributed_func;

//! \brief Have all cores execute the same function simultaneously.
//!
//! The cores used are from core 0 of the current tile onwards.
//!
//! \warning Must be executed on core 0 of the first tile in the group.
//!
//! \warning Overwrites channel map table entries 2 and 3, uses `CH_REGISTER_3`.
void loki_execute(const distributed_func* config);

//! \brief Wait for all tiles between 0 and (tiles-1) to reach this point before
//! continuing.
//!
//! This function may only be executed on core 0 of each tile.
//!
//! \warning Overwrites channel map table entry 2 and uses `CH_REGISTER_7`.
void loki_sync_tiles(const uint tiles);

//! \brief Wait for all cores between 0 and (cores-1) to reach this point before
//! continuing.
//! \param cores Total number of cores participating in sync.
//! \param first_tile First tile participating in sync.
//!
//! \warning Quite expensive/slow. Use sparingly.
//!
//! \warning Overwrites channel map table entry 2 and uses `CH_REGISTER_3` and `CH_REGISTER_7`.
void loki_sync_ex(const unsigned int cores, const tile_id_t first_tile);

//! \brief Wait for all cores between 0 and (cores-1) to reach this point before
//! continuing.
//! \param cores Total number of cores participating in sync.
//!
//! \warning Quite expensive/slow. Use sparingly.
//!
//! \warning Overwrites channel map table entry 2 and uses `CH_REGISTER_3` and `CH_REGISTER_7`.
static inline void loki_sync(const uint cores) {
  loki_sync_ex(cores, tile_id(1, 1));
}

//! \brief Wait for all cores between 0 and (cores-1) on a tile to reach this point before
//! continuing.
//!
//! Slightly faster synchronisation if all cores are on the same tile.
//! Also allows synchronisation which doesn't start at tile 0.
//!
//! \warning Overwrites channel map table entry 2 and uses `CH_REGISTER_3`.
void loki_tile_sync(const uint cores);

//! \brief Execute func on another core, and return the result to a given location.
//!
//! Note that due to various limitations of parameter passing, the function is
//! currently limited to having a maximum of five arguments.
//! For the moment, "another core" is always core 1.
//!
//! \warning Overwrites channel map table entries 2 and 3 and uses `CH_REGISTER_3`.
void loki_spawn(void* func, const channel_t return_address, const int argc, ...);

//! \brief Execute function on another core.
//!
//! Assumes that the remote core has already been initialised using `loki_init`.
//! If spawning on a remote tile, the content of `args` must be flushed first.
//!
//! \param tile Tile to execute the function on.
//! \param core Core on the given tile to execute the function on.
//! \param func Function to be executed. Must take a single `void*` argument or none at all.
//! \param args Struct containing all information required by the function.
//!
//! \warning Overwrites channel map table entry 2.
//! \warning If spawning on a remote tile, the content of `args` must first be flushed.
void loki_remote_execute(tile_id_t tile, core_id_t core, void* func, void* args);

//! A core will stop work if it executes this function.
void loki_sleep(void) __attribute__((__noreturn__));

#endif
