/*! \file channels.h
 * \brief Functions to deal directly with Loki channels. */

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

/* Historically this header included these, so we continue to do so for
 * compatibility. */

#include <loki/channel_io.h>
#include <loki/channel_map_table.h>

#endif
