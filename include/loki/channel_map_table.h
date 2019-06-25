/*! \file channel_map_table.h
 * \brief Functions to access the channel map table. 
 *
 * Most Loki instructions have the option to send their results onto the network
 * as well as storing them locally in the register file. The channel map table 
 * provides a layer of abstraction for this process. Instructions specify an
 * entry in the channel map table, and that entry describes how the data should
 * reach its destination(s).
 *
 * The channel map table can be updated in software at runtime, allowing very
 * flexible communication mechanisms. Network destinations can include memory
 * banks on the local tile, any core on the chip, or any subset of cores on the
 * local tile.
 *
 * Each core has its own private channel map table, and is responsible for
 * maintaining its contents. It behaves in a similar way to a register file, and
 * has its own ABI describing how the entries should be preserved:
 * [ABI](https://svr-rdm34-issue.cl.cam.ac.uk/w/loki/abi/#channel-map-table).
 *
 * To generate entries to be stored in the channel map table, see channels.h.
 */

#include <loki/channels.h>

#ifndef LOKI_CHANNEL_MAP_TABLE_H_
#define LOKI_CHANNEL_MAP_TABLE_H_

#include <assert.h>
#include <loki/chip.h>

//! \brief Get an entry from the channel map table.
static inline channel_t get_channel_map(int id) {
  channel_t result;
  asm volatile (
    "fetchr 0f\n"
    "getchmap %0, %1\n"        // need nop after getchmap to prevent
    "nor.eop r0, r0, r0\n0:\n" // undefined behaviour
    : "=r"(result)
    : "r"(id)
  );
  return result;
}

//! Set an entry in the channel map table.
static inline void set_channel_map(int id, channel_t value) {
  asm volatile (
    "fetchr 0f\n"
    "setchmap %0, %1\n"        // need nop after setchmap to prevent
    "nor.eop r0, r0, r0\n0:\n" // undefined behaviour
    :
    : "r"(value), "r"(id)
  );
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
  
  channel_t address = loki_mcast_address(single_core_bitmask(next_core), channel, false);
  set_channel_map(output, address);
}

//! \brief Save a channel map table entry to be later restored with \ref
//! channel_map_restore.
//!
//! \param id Channel to save.
//! \return A value to be pass to \ref channel_map_restore to restore the
//! channel's old value.
//!
//! Should be used for preserving callee saved channels.
static inline channel_t channel_map_save(int id) {
  return get_channel_map(id);
}

//! \brief Restore a saved channel map table entry, saved with \ref
//! channel_map_save or \ref channel_map_swap.
//!
//! \param id Channel to restore.
//! \param value Return value of \ref channel_map_save.
//!
//! Should be used for restoring callee saved channels.
static inline void channel_map_restore(int id, channel_t value) {
  set_channel_map(id, value);
}

//! \brief Replace a channel entry with a new one, returning the old one to be
//! restored later with \ref channel_map_restore.
//!
//! \param id Channel to update.
//! \param value New value to put in the channel map table.
//! \return A value to be pass to \ref channel_map_restore to restore the
//! channel's old value.
//!
//! Should be used for preserving callee saved channels.
static inline channel_t channel_map_swap(int id, channel_t value) {
  channel_t result;
  asm volatile (
    "fetchr 0f\n"
    "getchmap %0, %2\n"
    "setchmap %1, %2\n"
    "nor.eop r0, r0, r0\n0:\n"
    : "=&r"(result)
    : "r"(value), "r"(id)
  );
  return result;
}

#endif
