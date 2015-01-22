/*! \file channels.h
 * \brief Functions to deal directly with Loki channels. */

#ifndef LOKI_CHANNELS_H_
#define LOKI_CHANNELS_H_

#include <loki/control_registers.h>
#include <loki/types.h>

//! Communications channel address type.
typedef unsigned long channel_t;

//! Forms a channel address to talk to a particular tile component and channel.
static inline channel_t loki_core_address(const tile_id_t tile, const enum Components component,
                                          const enum Channels channel) {
  return ((channel_t)tile << 20) | ((channel_t)component << 12) | ((channel_t)channel << 8);
}
//! Forms a multicast channel address to talk to a number of components on a tile.
static inline channel_t loki_mcast_address(const tile_id_t tile, const enum MulticastDestinations bitmask,
                                           const enum Channels channel) {
  return (1 << 28) | loki_core_address(tile, (enum Components)bitmask, channel);
}
//! Forms a memory channel to allow memory accesses.
static inline channel_t loki_mem_address(const enum Channels returnChannel, const tile_id_t tile,
                                         const enum Components component, const enum Channels channel,
                                         const enum MemConfigGroupSize log2GroupSize, const enum MemConfigLineSize log2LineSize) {
  return ((channel_t)returnChannel << 29) | loki_core_address(tile, component, channel)
       | ((channel_t)log2GroupSize << 4) | (channel_t)log2LineSize;
}
//! Generate a memory configuration command. Send to a memory group as-is.
static inline int loki_mem_config(const enum MemConfigAssociativity log2Assoc, const enum MemConfigLineSize log2LineSize,
                                  const enum MemConfigType isCache, const enum MemConfigGroupSize log2GroupSize) {
  return (log2Assoc << 20) | (log2LineSize << 16) | (isCache << 8) | (log2GroupSize);
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

//! Return the minimum number of tiles required to hold the given number of cores.
static inline unsigned int num_tiles(const unsigned int cores) {
  return ((cores - 1)/CORES_PER_TILE) + 1;
}

//! Calculate the number of cores that are active on a given tile, given a total
//! number of active cores.
static inline unsigned int cores_this_tile(const unsigned int cores, const tile_id_t tile) {
  return (cores - tile * CORES_PER_TILE > CORES_PER_TILE)
       ? CORES_PER_TILE
       : cores - tile * CORES_PER_TILE;
}

//! Return a globally unique idenitifer for this core.
static inline int get_unique_core_id(void) {
  return get_control_register(CR_CPU_LOCATION);
}

//! Return the core's position within its tile.
static inline enum Components get_core_id() {
  return get_control_register(CR_CPU_LOCATION) & 0x7;
}

//! Return the ID of the tile the core is in.
static inline tile_id_t get_tile_id() {
  return get_control_register(CR_CPU_LOCATION) >> 4;
}

#include <loki/channel_io.h>
#include <loki/channel_map_table.h>

#endif
