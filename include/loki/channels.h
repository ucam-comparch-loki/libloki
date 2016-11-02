/*! \file channels.h
 * \brief Functions to deal directly with Loki channels.
 * 
 * Each Loki component has a number of input channel ends and output channel
 * ends. A channel connects one output channel end to one or more input channel
 * ends. The following restrictions are imposed:
 *  * Cores may only connect to memory banks on the same tile
 *  * Cores may connect with cores over the local network (same tile only) or
 *    global network (any tile)
 *    * The local network supports multicast to any subset of local cores
 *    * The global network supports credit-based flow control
 *
 * The functions in this file can be used to generate network addresses which
 * encode these different communication patterns. They are usually paired with
 * functions in channel_map_table.h to commit a channel configuration to a
 * core's channel map table. Data may only be sent on a channel once this
 * process is complete.
 * 
 * Each core is responsible for maintaining its own output channels, but is
 * oblivious to the channels connected to its inputs.
 */

#ifndef LOKI_CHANNELS_H_
#define LOKI_CHANNELS_H_

#include <loki/ids.h>
#include <loki/types.h>

//! Default amount of credits a channel receives in Loki. This value is deadlock
//! safe for arbitrary communication flows.
#define          DEFAULT_CREDIT_COUNT  4
//! Default amount of credits for a connection to the instruction FIFO. This
//! value is deadlock safe for arbitrary connection flows.
#define DEFAULT_IPK_FIFO_CREDIT_COUNT  8
//! Special infinite credit value.
#define         INFINITE_CREDIT_COUNT 63

//! \brief Returns the default (deadlock safe) credit count amount for a
//! connection to a particular destination channel.
//!
//! \param channel The index of a core's input channel end.
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

//! \brief Communications channel address type.
//!
//! Full documentation for the encoding can be found
//! [here](https://svr-rdm34-issue.cl.cam.ac.uk/w/loki/architecture/core/channel_map_table/).
typedef unsigned long channel_t;

//! \brief Returns the default (deadlock safe) credit count amount for a
//! connection to a particular destination channel.
//!
//! \param channel An encoded channel description.
static inline unsigned int loki_channel_default_credit_count(
    channel_t const channel
) {
  return loki_default_credit_count((enum Channels)((channel >> 2) & 7));
}

//! \brief Form a channel address to communicate with another core using a
//! connection with flow control.
//!
//! Specifying 0 credits leaves the credit counter the same as the previous
//! connection.
//!
//! While this address is valid for communicating between any two cores on the
//! chip, consider using the more-efficient \ref loki_mcast_address when the two
//! cores are known to be on the same tile, or \ref loki_core_address_ex when
//! the two cores may be on the same tile.
//!
//! \param tile The encoded ID of the tile holding the destination core.
//! \param core The position of the destination core within its tile.
//! \param channel The input channel end to access at the destination core.
//! \param credits The degree of flow control to use for this connection.
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

//! \brief Form a multicast channel address to talk to any number of components
//! on the local tile.
//!
//! \param bitmask Bitmask of cores to receive data. Least-significant bit
//! represents core 0. Any number of cores can be accessed, including the sender.
//! \param channel Input channel end to access at destination core(s). The same
//! channel must therefore be used at all cores.
//! \param pipelineStallMode When `true`, specifies that data being sent on this
//! channel cannot proceed to the WRITE stage until the NET stage is empty. Used
//! to enforce stricter synchronisation between cores: memory operations must
//! have reached the L1 cache before synchronisation messages can reach other
//! cores to notify them that the data has been updated.
static inline channel_t loki_mcast_address(const enum MulticastDestinations bitmask,
                                           const enum Channels              channel,
                                           const bool                       pipelineStallMode) {
  return (pipelineStallMode << 13) | (bitmask << 5) | (channel << 2);
}

//! \brief Form a memory channel to allow memory accesses, and determines the
//! way in which the memory should behave. Consider using the more-specific
//! \ref loki_cache_address or \ref loki_scratchpad_address when "simple"
//! behaviour is required.
//!
//! A memory channel groups a number of memory banks together, with cache lines
//! interleaved across them. All cores requiring coherent data access must use
//! consistent memory channels to ensure that this interleaving is identical.
//!
//! \param groupStart The first memory bank in the group.
//! \param returnCore The core to which data should be returned from memory. If
//! this is different to the requesting core, extra care must be taken to ensure
//! the receiving core can consume the data, otherwise deadlock may occur.
//! \param returnChannel The input channel end at the `returnCore` to which data
//! should be sent from memory.
//! \param groupSize The (encoded) number of memory banks to group together.
//! \param skipL1 All memory requests bypass the L1 cache and go straight to the
//! next level of memory hierarchy.
//! \param skipL2 All memory requests which reach the L2 cache (if there is one)
//! bypass it and go straight to main memory.
//! \param scratchpadL1 When `true`, L1 memory banks are accessed as scratchpads
//! rather than caches.
static inline channel_t loki_mem_address(const enum Memories           groupStart,
                                         const enum Cores              returnCore,
                                         const enum Channels           returnChannel,
                                         const enum MemConfigGroupSize groupSize,
                                         const bool                    skipL1,
                                         const bool                    skipL2,
                                         const bool                    scratchpadL1) {
  return (scratchpadL1 << 15) | (skipL2 << 14) | (skipL1 << 13)
       | (groupSize << 11) | (returnChannel << 8) | (groupStart << 5) | (returnCore << 2) | 2;
}

//! \brief Form a memory channel which accesses a group of local memory banks as
//! a cache.
//!
//! A cache channel groups a number of memory banks together, with cache lines
//! interleaved across them. All cores requiring coherent data access must use
//! consistent cache channels to ensure that this interleaving is identical.
//!
//! \param groupStart The first memory bank in the group.
//! \param returnCore The core to which data should be returned from memory. If
//! this is different to the requesting core, extra care must be taken to ensure
//! the receiving core can consume the data, otherwise deadlock may occur.
//! \param returnChannel The input channel end at the `returnCore` to which data
//! should be sent from memory.
//! \param groupSize The (encoded) number of memory banks to group together.
static inline channel_t loki_cache_address(
    const enum Memories           groupStart
  , const enum Cores              returnCore
  , const enum Channels           returnChannel
  , const enum MemConfigGroupSize groupSize
) {
  return loki_mem_address(
    groupStart, returnCore, returnChannel, groupSize, false, false, false
  );
}

//! \brief Form a memory channel which accesses a group of local memory banks as
//! a scratchpad.
//!
//! A scratchpad channel groups a number of memory banks together, with cache
//! lines interleaved across them. All cores requiring coherent data access must
//! use consistent scratchpad channels to ensure that this interleaving is
//! identical.
//!
//! \param groupStart The first memory bank in the group.
//! \param returnCore The core to which data should be returned from memory. If
//! this is different to the requesting core, extra care must be taken to ensure
//! the receiving core can consume the data, otherwise deadlock may occur.
//! \param returnChannel The input channel end at the `returnCore` to which data
//! should be sent from memory.
//! \param groupSize The (encoded) number of memory banks to group together.
static inline channel_t loki_scratchpad_address(
    const enum Memories           groupStart
  , const enum Cores              returnCore
  , const enum Channels           returnChannel
  , const enum MemConfigGroupSize groupSize
) {
  return loki_mem_address(
    groupStart, returnCore, returnChannel, groupSize, false, false, true
  );
}

//! \brief Form a channel address to talk to a particular core.
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

/* Historically this header included these, so we continue to do so for
 * compatibility. */

#include <loki/channel_io.h>
#include <loki/channel_map_table.h>

#endif
