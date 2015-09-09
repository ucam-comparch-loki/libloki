/*! \file channels.h
 * \brief Functions to deal directly with Loki channels. */

#ifndef LOKI_CHANNELS_H_
#define LOKI_CHANNELS_H_

#include <loki/control_registers.h>
#include <loki/types.h>

//! Default amount of credits a channel receives in Loki. This value is deadlock
//! safe for arbitrary communication flows.
#define          DEFAULT_CREDIT_COUNT  4
//! Default amount of credits for a connection to the instruction FIFO. This
//! value is deadlock safe for arbitrary connection flows.
#define DEFAULT_IPK_FIFO_CREDIT_COUNT  8
//! Special infinite credit value.
#define         INFINITE_CREDIT_COUNT 63

//! Returns the default (deadlock safe) credit count amount for a connection to
//! a particular destination channel.
static inline unsigned int loki_default_credit_count(
    enum Channels const channel
) {
  switch (channel) {
  case CH_IPK_FIFO:  return DEFAULT_IPK_FIFO_CREDIT_COUNT;
  case CH_IPK_CACHE:  return DEFAULT_CREDIT_COUNT;
  case CH_REGISTER_2: return DEFAULT_CREDIT_COUNT;
  case CH_REGISTER_3: return DEFAULT_CREDIT_COUNT;
  case CH_REGISTER_4: return DEFAULT_CREDIT_COUNT;
  case CH_REGISTER_5: return DEFAULT_CREDIT_COUNT;
  case CH_REGISTER_6: return DEFAULT_CREDIT_COUNT;
  case CH_REGISTER_7: return DEFAULT_CREDIT_COUNT;
  default: assert(0); __builtin_unreachable();
  }
}

//! Communications channel address type.
typedef unsigned long channel_t;

//! Returns the default (deadlock safe) credit count amount for a connection to
//! a particular destination channel.
static inline unsigned int loki_channel_default_credit_count(
    channel_t const channel
) {
  return loki_default_credit_count((enum Channels)((channel >> 2) & 7));
}

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
      , INFINITE_CREDIT_COUNT
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

//! It is common to have a core send data out to all other cores - it does not
//! need to send data to itself. num_cores is inclusive of current.
static inline enum MulticastDestinations all_cores_except_current(uint num_cores) {
  return all_cores(num_cores) & ~single_core_bitmask(get_core_id());
}

#include <loki/channel_io.h>
#include <loki/channel_map_table.h>

//! \brief Test if an asynchronous connection has completed.
//! \param id The channel that is connecting.
//! \returns Whether or not the channel is now connected.
//!
//! This method polls a channel end to see if it has connected. A connection
//! attempt should have already been started with \ref loki_connect_async. When
//! this method returns true, the connection is completed and the channel can be
//! safely used.
static inline bool loki_connect_async_poll(int const id) {
  // woche would block, defeating the point of the poll.
  channel_t const c = get_channel_map(id);

  // Check the credit count - have we got them all back?
  if (c >> 14 == loki_channel_default_credit_count(c)) {
    if (c & 0x2) {
      // Acquired bit is set - we're ready to go.
      return true;
    } else {
      loki_channel_acquire(id);
      return false;
    }
  } else {
    // Last attempt has not yet returned.
    return false;
  }
}

//! \brief Begins an asynchronous connection operation.
//! \param id The channel to connect.
//! \param tile The remote tile to connect to.
//! \param core The core on the tile to connect to.
//! \param channel The destination channel to connect to.
//!
//! This method overwrites the channel map table entry specified by id, and then
//! begins a connection procedure to that channel. The status of this connection
//! operation can be checked with \ref loki_connect_async_poll. Alternatively, a
//! core can sleep until the connection is completed with
//! \ref loki_connect_async_wait. A synchronous version of this operation is
//! available \ref loki_connect.
//!
//! \remark In order to ensure deadlock freedom, the credit count is set to
//! \ref loki_default_credit_count(channel) for the connection.
static inline void loki_connect_async(
    int const id
  , tile_id_t const tile
  , enum Cores const core
  , enum Channels const channel
) {
  channel_t const c =
    loki_core_address(tile, core, channel, loki_default_credit_count(channel));

  set_channel_map(id, c);

  loki_connect_async_poll(id);
}

//! \brief Wait for the end of an asynchronous connection operation.
//! \param id The channel to wait for.
//!
//! Sleeps the core until the asynchronous connection operation on the specified
//! channel is completed.
static inline void loki_connect_async_wait(int const id) {
  while (!loki_connect_async_poll(id)) {
    loki_channel_wait_empty(id);
  }
}

//! \brief Connects to the specified destination.
//! \param id The channel to connect.
//! \param tile The remote tile to connect to.
//! \param core The core on the tile to connect to.
//! \param channel The destination channel to connect to.
//!
//! This method overwrites the channel map table entry specified by id, and then
//! performs a connection procedure to that channel. An asynchronous version of
//! this operation is available \ref loki_connect_async.
//!
//! \remark In order to ensure deadlock freedom, the credit count is set to
//! \ref DEFAULT_CREDIT_COUNT for the connection.
static inline void loki_connect(
    int const id
  , tile_id_t const tile
  , enum Cores const core
  , enum Channels const channel
) {
  loki_connect_async(id, tile, core, channel);
  loki_connect_async_wait(id);
}

//! \brief Disconnects the specified channel.
//! \param id The channel to disconnect.
//!
//! This operation releases the specified channel, allowing another core to
//! connect to it. It first blocks until the channel is empty, in order to
//! ensure that no credits are outstanding.
static inline void loki_disconnect(int const id) {
  loki_channel_wait_empty(id);
  loki_channel_release(id);
}

#endif
