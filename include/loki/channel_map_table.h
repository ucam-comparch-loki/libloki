/*! \file channel_map_table.h
 * \brief Functions to access the channel map table. */

#ifndef LOKI_CHANNEL_MAP_TABLE_H_
#define LOKI_CHANNEL_MAP_TABLE_H_

#include <assert.h>
#include <loki/channels.h>

//! The number of entries in the channel map table, including reserved entries.
#define CHANNEL_MAP_TABLE_SIZE 16

//! Get an entry from the channel map table.
static inline channel_t get_channel_map(int id) {
  channel_t result;
  asm ("getchmap %0, %1\nfetchr.eop 0f\n0:\n" : "=r"(result) : "r"(id));
  return result;
}

//! Set an entry in the channel map table.
static inline void set_channel_map(int id, channel_t value) {
  asm ("setchmap %0, %1\nfetchr.eop 0f\n0:\n" : : "r"(value), "r"(id));
}

//! Connect this core's output to the specified other core.
static inline void loki_connect(int id, tile_id_t tile, enum Components component, enum Channels channel) {
  set_channel_map(id, loki_core_address(tile, component, channel));
}

//! \brief Get a core in a group to connect to another HELIX-style (wrapping around
//! within the group).
//!
//! \param output the entry in the channel map table to write (integer literal)
//! \param offset core n connects to core (n+offset mod group_size)
//! \param channel the input channel of the remote core to connect to
//! \param group_size the number of cores in the group. All cores must be contiguous
//!                   and start at position 0 in the tile
static inline void loki_connect_helix(int output, int offset, enum Channels channel, int group_size) {
  int this_core = get_core_id();
  int next_core = this_core + offset;
  if (next_core >= group_size)
    next_core -= group_size;
  if (next_core < 0)
    next_core += group_size;
  loki_connect(output, get_tile_id(), next_core, channel);
}

#endif
