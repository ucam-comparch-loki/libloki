/*! \file ids.h
 * \brief Functions to identify and count components on the Loki chip.
 *
 * Cores and memory banks will typically have a local ID and a global ID, and
 * possibly also a group ID.
 *  * Local ID resets to 0 with each new tile, and reaches a maximum of \ref
 * CORES_PER_TILE - 1 or \ref BANKS_PER_TILE - 1.
 *  * Global ID is unique across the whole chip.
 *  * Group ID is dynamic and represents a unique position among the collection
 * of similar components working together.
 *
 * Tiles are identified using their X and Y co-ordinates on-chip, with (0,0)
 * being the south-west corner. Tiles at all edges of the chip are reserved for
 * I/O, leaving only the central block of tiles to hold the cores and memory
 * banks. These *compute tiles* may additionally be identified by a single
 * integer - the global tile number.
 */

#ifndef LOKI_IDS_H_
#define LOKI_IDS_H_

#include <loki/control_registers.h>
#include <loki/types.h>

//! \brief Return the globally unique idenitifer for this core.
static inline core_id_t get_unique_core_id(void) __attribute__((const));
static inline core_id_t get_unique_core_id(void) {
  return (core_id_t)get_control_register(CR_CPU_LOCATION);
}

//! \brief Compute the globally unique identifier for an arbitrary core.
static inline core_id_t make_unique_core_id(tile_id_t tile, enum Cores core) {
  return (tile << 4) | (core_id_t)core;
}

//! \brief Extract the core part of a global core id.
static inline enum Cores get_unique_core_id_core(core_id_t id) {
  return id & 0x7;
}

//! \brief Extract the tile part of a global core id.
static inline tile_id_t get_unique_core_id_tile(core_id_t id) {
  return id >> 4;
}

//! \brief Return the core's position within its tile.
static inline enum Cores get_core_id() __attribute__((const));
static inline enum Cores get_core_id() {
  return get_unique_core_id_core(get_control_register(CR_CPU_LOCATION));
}

//! \brief Return the ID of the tile the core is in.
static inline tile_id_t get_tile_id() __attribute__((const));
static inline tile_id_t get_tile_id() {
  return get_unique_core_id_tile(get_control_register(CR_CPU_LOCATION));
}

//! \brief Encode a tile's position from its coordinates.
static inline tile_id_t tile_id(const unsigned int x, const unsigned int y) {
  return (x << 3) | (y << 0);
}

//! \brief Convert a tile_id_t into a global tile number (useful for iterating).
static inline unsigned int tile2int(const tile_id_t tile) {
  unsigned int x = (tile >> 3) - 1;
  unsigned int y = (tile & 7) - 1;
  return (y * COMPUTE_TILE_COLUMNS) + x;
}

//! \brief Convert a global tile number into the tile_id_t type.
static inline tile_id_t int2tile(const unsigned int val) {
  return tile_id((val % COMPUTE_TILE_COLUMNS) + 1, (val / COMPUTE_TILE_COLUMNS) + 1);
}

//! \brief Generate a bitmask for a single core on the local tile.
//!
//! The result is in the format required by \ref loki_mcast_address.
static inline enum MulticastDestinations single_core_bitmask(enum Cores core) {
  return (enum MulticastDestinations)(1 << core);
}

//! \brief Generate a bitmask representing the first `num_cores` cores on the
//! local tile.
//!
//! The result is in the format required by \ref loki_mcast_address.
static inline enum MulticastDestinations all_cores(uint num_cores) {
  return (enum MulticastDestinations)((1 << num_cores) - 1);
}

//! \brief Generate a bitmask representing the first `num_cores` cores on the
//! local tile, but excluding core 0. Used to allow core 0 to send data to
//! the remaining `num_cores - 1` other cores.
//!
//! The result is in the format required by \ref loki_mcast_address.
static inline enum MulticastDestinations all_cores_except_0(uint num_cores) {
  return all_cores(num_cores) & ~MULTICAST_CORE_0;
}

//! \brief Generate a bitmask representing the first `num_cores` cores on the
//! local tile, but excluding this core. Used to allow this core to send data to
//! the remaining `num_cores - 1` other cores.
//!
//! The result is in the format required by \ref loki_mcast_address.
static inline enum MulticastDestinations all_cores_except_current(uint num_cores) {
  return all_cores(num_cores) & ~single_core_bitmask(get_core_id());
}

//! \brief Compute the minimum number of tiles required to hold the given number
//! of cores.
static inline unsigned int num_tiles(const unsigned int cores) {
  return ((cores - 1)/CORES_PER_TILE) + 1;
}

//! \brief Calculate the number of cores that are active on a given tile, given
//! a total number of active cores.
//! \param cores Total number of cores executing.
//! \param tile ID of this tile.
//! \param first_tile ID of first tile in the group.
static inline unsigned int cores_this_tile(
    const unsigned int cores
  , const tile_id_t tile
  , const tile_id_t first_tile
) {
  return (cores - (tile2int(tile) - tile2int(first_tile)) * CORES_PER_TILE > CORES_PER_TILE)
       ? CORES_PER_TILE
       : cores - (tile2int(tile) - tile2int(first_tile)) * CORES_PER_TILE;
}

//! \brief Given a continuous group of cores starting at first_core, compute
//! the index of this core.
static inline unsigned int group_core_index(
    core_id_t const first_core_id
) {
  core_id_t const my_id = get_unique_core_id();

  unsigned int const my_tile = tile2int(get_unique_core_id_tile(my_id));
  unsigned int const first_tile = tile2int(get_unique_core_id_tile(first_core_id));
  unsigned int const my_core = get_unique_core_id_core(my_id);
  unsigned int const first_core = get_unique_core_id_core(first_core_id);

  return (my_tile - first_tile) * CORES_PER_TILE + my_core - first_core;
}

//! \brief Given a continuous group of cores starting at first_core, compute the
//! id of the core with given index.
static inline core_id_t group_core_id(
    core_id_t const first_core_id
  , unsigned int const index
) {
  unsigned int const first_tile = tile2int(get_unique_core_id_tile(first_core_id));
  unsigned int const first_core = get_unique_core_id_core(first_core_id);

  if (CORES_PER_TILE > first_core + index) {
    return make_unique_core_id(
        get_unique_core_id_tile(first_core_id)
      , first_core + index
    );
  }

  return make_unique_core_id(
      int2tile(first_tile + (index + first_core) / CORES_PER_TILE)
    , (first_core + index) % CORES_PER_TILE
  );
}

#endif
