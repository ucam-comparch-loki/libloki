/*! \file deprecated.h
 * \brief Old functions and macros kept for compatibility which should be avoided. */

#ifndef LOKI_DEPRECATED_H_
#define LOKI_DEPRECATED_H_

#include <loki/channel_map_table.h>
#include <loki/init.h>
#include <loki/scratchpad.h>
#include <loki/types.h>

//! \brief Prepare a given number of cores to execute code later in the program.
//!
//! This must be the first thing executed at the start of the program (for now).
//!
//! \deprecated
//! Use loki_init_default(num_cores, 0) instead.
static inline void init_cores(const int num_cores) {
  loki_init_default(num_cores, 0);
}

//! \deprecated type for memory configurations. Use specific types like \ref MemConfigType instead.
typedef int MemConfig;

//! \brief Set a channel map entry to the value in the named C variable. entry must
//! be an integer literal.
//! \deprecated Use \ref set_channel_map instead.
#define SET_CHANNEL_MAP(entry, address) set_channel_map(entry, address)

//! \brief Connect an output to a channel of a remote core or memory.
//!
//! Note that output must be an integer literal.
//! \deprecated Use \ref loki_connect instead.
#define CONNECT(id, tile, component, channel) loki_connect(id, tile, component, channel)

//! \brief Get a core in a group to connect to another HELIX-style (wrapping around
//! within the group).
//!
//! \deprecated Use \ref loki_connect_helix instead.
#define HELIX_CONNECT(output, offset, channel, group_size) loki_connect_helix(output, offset, channel, group_size)

//! \brief Old memory setup macro.
//!
//! \deprecated No longer necessary, do not use.
//!
//! Set up a connection with memory. It is assumed that the given output channel
//! already points to the memory, and this command tells the memory where to
//! send data back to.
//! If data is sent back to a data input, the core must consume a synchronisation
//! message which confirms that all memories are now set up properly.
#define MEMORY_CONNECT(output, return_address) (void)

//! \brief Kill the core at the end of the given channel by sending it a nxipk command.
//!
//! This will break it out of infinite loops, and stop it waiting for data, if
//! necessary.
//!
//! \deprecated Use \ref loki_send_interrupt instead.
#define KILL(output) RMTNXIPK(output)

//! \brief Send a token (0) on a given output channel.
//! \param channel_map_entry must be an integer literal.
//!
//! \deprecated Use \ref loki_send_token instead.
#define SEND_TOKEN(channel_map_entry) SEND(0, channel_map_entry)

//! \brief Receive a token (and discard it) from a given register-mapped input.
//! \param register must be an integer literal.
//!
//! \deprecated Use \ref loki_receive_token instead.
#define RECEIVE_TOKEN(register) {\
  asm volatile (\
    "fetchr 0f\n"\
    "addu.eop r0, r" #register ", r0\n0:\n"\
    ::: /* no inputs, outputs, or clobbered registers */\
  );\
}

//! \brief Store multiple data values in the core's local scratchpad.
//! \param data pointer to the data
//! \param len the number of elements to store
//! \param address the position in the core's local scratchpad file to store the first value
//!
//! \deprecated Use \ref scratchpad_write_bytes instead.
static inline void scratchpad_block_store_chars(const char* data, size_t len, unsigned int address) {
  scratchpad_write_bytes(address * 4, data, len);
}
//! \brief Store multiple data values in the core's local scratchpad.
//! \param data pointer to the data
//! \param len the number of elements to store
//! \param address the position in the core's local scratchpad file to store the first value
//!
//! \deprecated Use \ref scratchpad_write_bytes instead.
static inline void scratchpad_block_store_shorts(const short* data, size_t len, unsigned int address) {
  scratchpad_write_bytes(address * 4, data, len * 2);
}
//! \brief Store multiple data values in the core's local scratchpad.
//! \param data pointer to the data
//! \param len the number of elements to store
//! \param address the position in the core's local scratchpad file to store the first value
//!
//! \deprecated Use \ref scratchpad_write_words instead.
static inline void scratchpad_block_store_ints(const int* data, size_t len, unsigned int address) {
  scratchpad_write_words(address, data, len);
}

#endif
