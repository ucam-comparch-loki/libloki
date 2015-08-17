/*! \file channels.h
 * \brief Functions to deal directly with Loki channels. */

#ifndef LOKI_CHANNELS_H_
#define LOKI_CHANNELS_H_

#include <loki/control_registers.h>
#include <loki/types.h>

//! Communications channel address type.
typedef unsigned long channel_t;

//! Encode a tile's position from its coordinates.
static inline tile_id_t tile_id(const unsigned int x, const unsigned int y) {
  return (x << 3) | (y << 0);
}

//! Convert a tile_id_t into a global tile number (useful for iterating).
static inline unsigned int tile2int(const tile_id_t tile) {
  unsigned int x = (tile >> 3) - 1;
  unsigned int y = (tile & 7) - 1;
  return (y * COMPUTE_TILE_COLUMNS) + x;
}

//! Convert a global tile number into the tile_id_t type.
static inline tile_id_t int2tile(const unsigned int val) {
  return tile_id((val % COMPUTE_TILE_COLUMNS) + 1, (val / COMPUTE_TILE_COLUMNS) + 1);
}

//! Forms a channel address to talk to a particular tile component and channel.
//! Specifying 0 credits leaves the credit counter the same as the previous connection.
static inline channel_t loki_core_address(const tile_id_t       tile,
                                          const enum Cores      core,
                                          const enum Channels   channel,
                                          const unsigned int    credits) {
  channel_t address = ((channel_t)tile << 8) | ((channel_t)core << 5) 
                    | ((channel_t)channel << 2) | 1;
  
  if (credits > 0)
    address = address | ((channel_t)credits << 14) | (1 << 20);
  
  return address;
}
//! Forms a multicast channel address to talk to a number of components on a tile.
//! Pipeline stall mode specifies that data being sent on this channel cannot
//! proceed to the WRITE stage until the NET stage is empty.
static inline channel_t loki_mcast_address(const enum MulticastDestinations bitmask,
                                           const enum Channels              channel,
                                           const bool                       pipelineStallMode) {
  return (pipelineStallMode << 13) | (bitmask << 5) | (channel << 2);
}
//! Forms a memory channel to allow memory accesses.
static inline channel_t loki_mem_address(const enum Memories           groupStart,
                                         const enum Cores              returnCore,
                                         const enum Channels           returnChannel,
                                         const enum MemConfigGroupSize groupSize,
                                         const bool                    skipL1,
                                         const bool                    skipL2,
                                         const bool                    scratchpadL1,
                                         const bool                    scratchpadL2) {
  return (scratchpadL2 << 16) | (scratchpadL1 << 15) | (skipL2 << 14) | (skipL1 << 13)
       | (groupSize << 11) | (returnChannel << 8) | (groupStart << 5) | (returnCore << 2) | 2;
}

//! Forms a memory channel which accesses a group of local cache banks as a
//! cache.
static inline channel_t loki_cache_address(
    const enum Memories           groupStart
  , const enum Cores              returnCore
  , const enum Channels           returnChannel
  , const enum MemConfigGroupSize groupSize
) {
  return loki_mem_address(
    groupStart, returnCore, returnChannel, groupSize, false, false, false, false
  );
}

//! Forms a memory channel which accesses a group of local cache banks as a
//! scratchpad.
static inline channel_t loki_scratchpad_address(
    const enum Memories           groupStart
  , const enum Cores              returnCore
  , const enum Channels           returnChannel
  , const enum MemConfigGroupSize groupSize
) {
  return loki_mem_address(
    groupStart, returnCore, returnChannel, groupSize, false, false, true, false
  );
}

//! Generate a bitmask with the lowest num_cores bits set to 1. This can then be
//! passed to \ref loki_mcast_address.
static inline enum MulticastDestinations all_cores(uint num_cores) {
  return (enum MulticastDestinations)((1 << num_cores) - 1);
}

//! It is common to have core 0 send data out to all other cores - it does not
//! need to send data to itself. num_cores is inclusive of 0.
static inline enum MulticastDestinations all_cores_except_0(uint num_cores) {
  return all_cores(num_cores) & ~MULTICAST_CORE_0;
}

//! Set up a bitmask for a single core on the local tile.
static inline enum MulticastDestinations single_core_bitmask(enum Cores core) {
  return (enum MulticastDestinations)(1 << core);
}

//! Return the minimum number of tiles required to hold the given number of cores.
static inline unsigned int num_tiles(const unsigned int cores) {
  return ((cores - 1)/CORES_PER_TILE) + 1;
}

//! Calculate the number of cores that are active on a given tile, given a total
//! number of active cores.
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

//! Return a globally unique idenitifer for this core.
static inline core_id_t get_unique_core_id(void) {
  return (core_id_t)get_control_register(CR_CPU_LOCATION);
}

//! Create a globally unique identifier for this core.
static inline core_id_t make_unique_core_id(tile_id_t tile, enum Cores core) {
  return (tile << 4) | (core_id_t)core;
}

//! Extract the core part of a global core id.
static inline enum Cores get_unique_core_id_core(core_id_t id) {
  return id & 0x7;
}

//! Extract the tile part of a global core id.
static inline tile_id_t get_unique_core_id_tile(core_id_t id) {
  return id >> 4;
}

//! Forms a channel address to talk to a particular core.
//! \param core The core to connect to. If this is less than 8, the core is
//!             assumed to be local rather than remote. Otherwise, a intertile
//!             connection is used, uncredited.
//! \param channel Channel to connect to.
static inline channel_t loki_core_address_ex(
    const core_id_t       core
  , const enum Channels   channel
) {
  if (core < 8)
    return loki_mcast_address(single_core_bitmask(core), channel, false);
  else
    return loki_core_address(
        get_unique_core_id_tile(core)
      , get_unique_core_id_core(core)
      , channel
      , 63
      );
}

//! Return the core's position within its tile.
static inline enum Cores get_core_id() {
  return get_unique_core_id_core(get_control_register(CR_CPU_LOCATION));
}

//! Return the ID of the tile the core is in.
static inline tile_id_t get_tile_id() {
  return get_unique_core_id_tile(get_control_register(CR_CPU_LOCATION));
}

#include <loki/channel_io.h>
#include <loki/channel_map_table.h>

#endif
