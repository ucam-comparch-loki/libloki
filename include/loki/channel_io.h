/*! \file channel_io.h
 * \brief Functions to send and receive messages on channels. */

#ifndef LOKI_CHANNEL_IO_H_
#define LOKI_CHANNEL_IO_H_

#include <assert.h>
#include <loki/channels.h>
#include <loki/channel_map_table.h>
#include <loki/control_registers.h>
#include <loki/sendconfig.h>
#include <loki/types.h>
#include <string.h>

//! \brief Send the value of the named variable on the given output channel.
//! \param variable value to send.
//! \param output must be an integer literal.
//!
//! Use \ref loki_send instead where possible.
#define SEND(variable, output) {\
  asm (\
    "fetchr 0f\n"\
    "addu.eop r0, %0, r0 -> " #output "\n0:\n"\
    :\
    : "r" ((int)variable)\
    :\
  );\
}

//! Send a value on a given output channel.
static inline void loki_send(const int channel, int value) {
  switch (channel) {
  case 0: SEND(value, 0); return;
  case 1: SEND(value, 1); return;
  case 2: SEND(value, 2); return;
  case 3: SEND(value, 3); return;
  case 4: SEND(value, 4); return;
  case 5: SEND(value, 5); return;
  case 6: SEND(value, 6); return;
  case 7: SEND(value, 7); return;
  case 8: SEND(value, 8); return;
  case 9: SEND(value, 9); return;
  case 10: SEND(value, 10); return;
  case 11: SEND(value, 11); return;
  case 12: SEND(value, 12); return;
  case 13: SEND(value, 13); return;
  case 14: SEND(value, 14); return;
  default: assert(0); __builtin_unreachable();
  }
}

//! \brief Receive a value from a particular input and store it in the named variable.
//! \param variable must be an lvalue.
//! \param input must be an integer literal.
//!
//! Use \ref loki_receive instead where possible.
#define RECEIVE(variable, input) {\
  asm volatile (\
    "fetchr 0f\n"\
    "addu.eop %0, r" #input ", r0\n0:\n"\
    : "=r" (variable)\
    :\
    :\
  );\
}

//! Receive a value from a given input channel.
static inline int loki_receive(const enum Channels channel) {
  int value;
  switch (channel) {
  case CH_REGISTER_2: RECEIVE(value, 2); return value;
  case CH_REGISTER_3: RECEIVE(value, 3); return value;
  case CH_REGISTER_4: RECEIVE(value, 4); return value;
  case CH_REGISTER_5: RECEIVE(value, 5); return value;
  case CH_REGISTER_6: RECEIVE(value, 6); return value;
  case CH_REGISTER_7: RECEIVE(value, 7); return value;
  default: assert(0); __builtin_unreachable();
  }
}

//! \brief Send an entire struct to another core.
//! \param struct_ptr structure to send. Must be word aligned.
//! \param bytes number of bytes. Must be a multiple of 4.
//! \param output must be an integer literal.
//!
//! Use \ref loki_send_data instead where possible.
#define SEND_STRUCT(struct_ptr, bytes, output) {\
  int end_of_struct;\
  void* struct_ptr_copy;\
  asm volatile (\
    "fetchr 0f\n"\
    "addui %1, %2, 4\n"\
    "addu.eop %0, %2, %3\n"\
    "0:\n"\
    "ldw -4(%1) -> 1\n"\
    "setlt.p r0, %1, %0\n"\
    "or r0, r2, r0 -> " #output "\n"\
    "psel.fetchr 0b, 0f\n"\
    "addui.eop %1, %1, 4\n"\
    "0:\n"\
    : "+&r" (end_of_struct), "+&r" (struct_ptr_copy)\
    : "r" (struct_ptr), "r" (bytes)\
    : \
  );\
}

//! \brief Send an entire array to another core.
//! \param data structure to send. Must be 4 byte aligned.
//! \param size number of words.
//! \param output channel to send on.
static inline void loki_send_words(const int *data, size_t size, int output) {
  switch (output) {
  case 0: SEND_STRUCT(data, size * 4, 0); return;
  case 1: SEND_STRUCT(data, size * 4, 1); return;
  case 2: SEND_STRUCT(data, size * 4, 2); return;
  case 3: SEND_STRUCT(data, size * 4, 3); return;
  case 4: SEND_STRUCT(data, size * 4, 4); return;
  case 5: SEND_STRUCT(data, size * 4, 5); return;
  case 6: SEND_STRUCT(data, size * 4, 6); return;
  case 7: SEND_STRUCT(data, size * 4, 7); return;
  case 8: SEND_STRUCT(data, size * 4, 8); return;
  case 9: SEND_STRUCT(data, size * 4, 9); return;
  case 10: SEND_STRUCT(data, size * 4, 10); return;
  case 11: SEND_STRUCT(data, size * 4, 11); return;
  case 12: SEND_STRUCT(data, size * 4, 12); return;
  case 13: SEND_STRUCT(data, size * 4, 13); return;
  case 14: SEND_STRUCT(data, size * 4, 14); return;
  default: assert(0); __builtin_unreachable();
  }
}
//! \brief Send an entire struct to another core.
//! \param data structure to send.
//! \param size number of bytes.
//! \param output channel to send on.
static inline void loki_send_data(const void *data, size_t size, int output) {
  // Reads must be aligned - unaligned data must be copied.
  if (((int)data & 3) || (size & 3)) {
    int buffer[(size + 3) / 4];
    buffer[(size - 1) / 4] = 0;
    memcpy(buffer, data, size);
    loki_send_words(buffer, (size + 3) / 4, output);
  } else {
    loki_send_words((const int *)data, size / 4, output);
  }
}

//! \brief Receive an entire struct over the network.
//! \param struct_ptr destination buffer. Must be word aligned.
//! \param bytes number of bytes. Must be a multiple of 4.
//! \param input must be an integer literal.
//!
//! Use \ref loki_receive_data instead where possible.
#define RECEIVE_STRUCT(struct_ptr, bytes, input) {\
  int end_of_struct;\
  void* struct_ptr_copy;\
  asm volatile (\
    "fetchr 0f\n"\
    "addui %1, %2, 4\n"\
    "addu.eop %0, %2, %3\n"\
    "0:\n"\
    "stw r" #input ", -4(%1) -> 1\n"\
    "setlt.p r0, %1, %0\n"\
    "psel.fetchr 0b, 0f\n"\
    "addui.eop %1, %1, 4\n"\
    "0:\n"\
    : "+&r" (end_of_struct), "+&r" (struct_ptr_copy)\
    : "r" (struct_ptr), "r" (bytes)\
    :\
  );\
}

//! \brief Receive an entire array from another core.
//! \param data buffer to fill. Must be 4 byte aligned.
//! \param size number of words.
//! \param input channel to receive on.
static inline void loki_receive_words(int *data, size_t size, enum Channels input) {
  switch (input) {
  case CH_REGISTER_2: RECEIVE_STRUCT(data, size * 4, 2); return;
  case CH_REGISTER_3: RECEIVE_STRUCT(data, size * 4, 3); return;
  case CH_REGISTER_4: RECEIVE_STRUCT(data, size * 4, 4); return;
  case CH_REGISTER_5: RECEIVE_STRUCT(data, size * 4, 5); return;
  case CH_REGISTER_6: RECEIVE_STRUCT(data, size * 4, 6); return;
  case CH_REGISTER_7: RECEIVE_STRUCT(data, size * 4, 7); return;
  default: assert(0); __builtin_unreachable();
  }
}

//! \brief Receive an entire struct from another core.
//! \param data buffer to fill.
//! \param size number of bytes.
//! \param input channel to receive on.
static inline void loki_receive_data(void *data, size_t size, enum Channels input) {
  // Reads must be aligned - unaligned data must be copied.
  if (((int)data & 3) || (size & 3)) {
    int buffer[(size + 3) / 4];
    loki_receive_words(buffer, (size + 3) / 4, input);
    memcpy(data, buffer, size);
  } else {
    loki_receive_words((int *)data, size / 4, input);
  }
}

//! \brief Send a rmtnxipk interrupt to a given channel.
//! \param output must be an integer literal.
//!
//! Use \ref loki_send_interrupt instead where possible.
#define RMTNXIPK(output) {\
  asm (\
    "fetchr 0f\n"\
    "rmtnxipk.eop -> " #output "\n0:\n"\
    :::\
  );\
}

//! \brief Send an interrupt on a given output channel.
//!
//! The channel must be aimed at another core's IPK fifo.
//! This will cause the core to move to the next packet as if a .eop instruction was encountered.
//! Be careful of race conditions when using this instruction!
static inline void loki_send_interrupt(const int channel) {
  switch (channel) {
  case 0: RMTNXIPK(0); return;
  case 1: RMTNXIPK(1); return;
  case 2: RMTNXIPK(2); return;
  case 3: RMTNXIPK(3); return;
  case 4: RMTNXIPK(4); return;
  case 5: RMTNXIPK(5); return;
  case 6: RMTNXIPK(6); return;
  case 7: RMTNXIPK(7); return;
  case 8: RMTNXIPK(8); return;
  case 9: RMTNXIPK(9); return;
  case 10: RMTNXIPK(10); return;
  case 11: RMTNXIPK(11); return;
  case 12: RMTNXIPK(12); return;
  case 13: RMTNXIPK(13); return;
  case 14: RMTNXIPK(14); return;
  default: assert(0); __builtin_unreachable();
  }
}

//! Send a token (0) on a given output channel.
static inline void loki_send_token(int channel) {
  loki_send(channel, 0);
}

//! Receive a token (and discard it) from a given input.
static inline void loki_receive_token(enum Channels channel) {
  loki_receive(channel);
}

//! \brief Wait for inputs on the specified channels. Return the channel number.
//! \param variable must be an lvalue.
//! \param mask must be an integer literal.
//!
//! If multiple inputs are available, select one in a fair round robin fashion.
//! The mask is defined such that a 1 in bit position 0 selects the r2 channel.
//!
//! Use \ref loki_wait_input instead wherever possible.
#define SELCH(variable, mask) {\
  asm volatile (\
    "fetchr 0f\n"\
    "selch.eop %0, " #mask "\n0:"\
    : "=r" (channel)\
    :\
    :\
  );\
}

//! \brief Wait for input on any register-mapped input channel and return the
//! channel number.
//!
//! If multiple inputs are available, select one in a fair round robin fashion.
static inline enum Channels loki_wait_input(void) {
  enum Channels channel;
  SELCH(channel, 0x3F);
  return channel;
}

//! \brief Wait for input on any register-mapped input channel, and return the value
//! which arrived.
static inline int receive_any_input() {
  return loki_receive(loki_wait_input());
}

//! \brief Store 1 in variable if there is any data in the given input buffer.
//! Store 0 otherwise. Note that buffer 0 corresponds to register 2.
//! \param variable must be an lvalue.
//! \param input must be an integer literal.
//!
//! Use \ref loki_test_channel instead where possible.
#define TEST_CHANNEL(variable, input) {\
  asm volatile (\
    "fetchr 0f\n"\
    "tstchi.eop %0, " #input "\n0:\n"\
    : "=r" (variable)\
    :\
    :\
  );\
}

//! \brief Return 1 if there is any data in the input buffer for the specified
//! channel, or 0 otherwise.
static inline int loki_test_channel(enum Channels channel) {
  int result;
  switch (channel) {
  case 2: TEST_CHANNEL(result, 0); return result;
  case 3: TEST_CHANNEL(result, 1); return result;
  case 4: TEST_CHANNEL(result, 2); return result;
  case 5: TEST_CHANNEL(result, 3); return result;
  case 6: TEST_CHANNEL(result, 4); return result;
  case 7: TEST_CHANNEL(result, 5); return result;
  default: assert(0); __builtin_unreachable();
  }
}

// Internal implementation - forces macro expansion.
#define SENDCONFIGi(variable, immediate, output) {\
  asm (\
    "fetchr 0f\n"\
    "sendconfig.eop %0, " #immediate " -> " #output "\n0:\n"\
    :\
    : "r" ((int)variable)\
    : "memory"\
  );\
}

//! \brief Sendconfig the value of the named variable on the given output channel.
//! \param variable value to send.
//! \param immediate out of band bits for message, must be an integer literal.
//! \param output must be an integer literal.
//!
//! Use functions instead of this macro where possible.
#define SENDCONFIG(variable, immediate, output) SENDCONFIGi(variable, immediate, output)

// Internal implementation - forces macro expansion.
#define SENDCONFIG2i(variable0, immediate0, variable1, immediate1, output) {\
  asm (\
     /* No special care needed - IFetch always emits both sendconfig
      * instructions before either complete. */ \
    "fetchr 0f\n"\
    "sendconfig %0, " #immediate0 " -> " #output "\n"\
    "sendconfig.eop %1, ((" #immediate1 ") | 1) -> " #output "\n0:\n"\
    :\
    : "r" ((int)variable0), "r" ((int)variable1)\
    : "memory"\
  );\
}

//! \brief Sendconfig 2 words to form a packet.
//! \param variable0 value to send first.
//! \param immediate0 out of band bits for first flit, must be an integer
//!                   literal.
//! \param variable1 value to send second.
//! \param immediate1 out of band bits for second flit, must be an integer
//!                   literal. This macro will force the tail bit to be set.
//! \param output must be an integer literal.
//!
//! Use functions instead of this macro where possible.
//!
//! This function is guaranteed not to cause deadlock even in the case that the
//! instruction memory bank is the target for the secondfig.
#define SENDCONFIG2(variable0, immediate0, variable1, immediate1, output) SENDCONFIG2i(variable0, immediate0, variable1, immediate1, output)

// Internal implementation - forces macro expansion.
#define SENDCONFIG9i( \
    variable0 \
  , immediate_head \
  , variable1 \
  , variable2 \
  , variable3 \
  , variable4 \
  , variable5 \
  , variable6 \
  , variable7 \
  , variable8 \
  , immediate_payload \
  , output \
) {\
  asm (\
     /* Ensure first sendconfig is the last instruction in a cache line. This
      * means IFetch will have already requested the next line before said first
      * sendconfig gets a chance to deadlock the bank. */\
    "fetchr.eop 0f\n"\
    ".p2align 5\n"\
    ".skip 0x18\n"\
    "0:\n"\
    "fetchr 0f\n"\
    "sendconfig %0, " #immediate_head " -> " #output "\n"\
    "sendconfig %1, ((" #immediate_payload ") & ~1) -> " #output "\n"\
    "sendconfig %2, ((" #immediate_payload ") & ~1) -> " #output "\n"\
    "sendconfig %3, ((" #immediate_payload ") & ~1) -> " #output "\n"\
    "sendconfig %4, ((" #immediate_payload ") & ~1) -> " #output "\n"\
    "sendconfig %5, ((" #immediate_payload ") & ~1) -> " #output "\n"\
    "sendconfig %6, ((" #immediate_payload ") & ~1) -> " #output "\n"\
    "sendconfig %7, ((" #immediate_payload ") & ~1) -> " #output "\n"\
    "sendconfig.eop %8, ((" #immediate_payload ") | 1) -> " #output "\n"\
    "0:\n"\
    :\
    : "r" ((int)variable0)\
    , "r" ((int)variable1)\
    , "r" ((int)variable2)\
    , "r" ((int)variable3)\
    , "r" ((int)variable4)\
    , "r" ((int)variable5)\
    , "r" ((int)variable6)\
    , "r" ((int)variable7)\
    , "r" ((int)variable8)\
    : "memory"\
  );\
}

//! \brief Sendconfig 9 words to form a flit.
//! \param variable0 value to send first.
//! \param immediate_head out of band bits for first flit, must be an integer
//!                       literal.
//! \param variable1 value to send second.
//! \param variable2 value to send third.
//! \param variable3 value to send fourth.
//! \param variable4 value to send fifth.
//! \param variable5 value to send sixth.
//! \param variable6 value to send seventh.
//! \param variable7 value to send eight.
//! \param variable8 value to send ninth.
//! \param immediate_payload out of band bits for remaining messages, must be
//!                          an integer literal. This macro forces the tail bit
//!                          to be set only on the last flit.
//! \param output must be an integer literal.
//!
//! Use functions instead of this macro where possible.
//!
//! This function is guaranteed not to cause deadlock even in the case that the
//! instruction memory bank is the target for the secondfig.
#define SENDCONFIG9( \
    variable0 \
  , immediate_head \
  , variable1 \
  , variable2 \
  , variable3 \
  , variable4 \
  , variable5 \
  , variable6 \
  , variable7 \
  , variable8 \
  , immediate_payload \
  , output \
) SENDCONFIG9i( \
    variable0 \
  , immediate_head \
  , variable1 \
  , variable2 \
  , variable3 \
  , variable4 \
  , variable5 \
  , variable6 \
  , variable7 \
  , variable8 \
  , immediate_payload \
  , output \
)

//! \brief Send load word command at given address on the given output channel.
//! \param address address to send.
//! \param output must be an integer literal.
//!
//! Use \ref loki_load_word instead where possible.
#define LDW(address, output) {\
  asm (\
    "fetchr 0f\n"\
    "ldw.eop 0(%0) -> " #output "\n0:\n"\
    :\
    : "r" ((void *)address)\
    : "memory"\
  );\
}

//! Send a load word memory operation on a given output channel.
static inline void loki_channel_load_word(
  const int channel, void const *address
) {
  switch (channel) {
  case 0: LDW(address, 0); return;
  case 1: LDW(address, 1); return;
  case 2: LDW(address, 2); return;
  case 3: LDW(address, 3); return;
  case 4: LDW(address, 4); return;
  case 5: LDW(address, 5); return;
  case 6: LDW(address, 6); return;
  case 7: LDW(address, 7); return;
  case 8: LDW(address, 8); return;
  case 9: LDW(address, 9); return;
  case 10: LDW(address, 10); return;
  case 11: LDW(address, 11); return;
  case 12: LDW(address, 12); return;
  case 13: LDW(address, 13); return;
  case 14: LDW(address, 14); return;
  default: assert(0); __builtin_unreachable();
  }
}

//! \brief Send load linked command at given address on the given output channel.
//! \param address address to send.
//! \param output must be an integer literal.
//!
//! Use \ref loki_load_linked instead where possible.
#define LDL(address, output) {\
  asm (\
    "fetchr 0f\n"\
    "ldl.eop %0, 0 -> " #output "\n0:\n"\
    :\
    : "r" ((void *)address)\
    : "memory"\
  );\
}

//! Send a load linked memory operation on a given output channel.
static inline void loki_channel_load_linked(
  const int channel, void const *address
) {
  switch (channel) {
  case 0: LDL(address, 0); return;
  case 1: LDL(address, 1); return;
  case 2: LDL(address, 2); return;
  case 3: LDL(address, 3); return;
  case 4: LDL(address, 4); return;
  case 5: LDL(address, 5); return;
  case 6: LDL(address, 6); return;
  case 7: LDL(address, 7); return;
  case 8: LDL(address, 8); return;
  case 9: LDL(address, 9); return;
  case 10: LDL(address, 10); return;
  case 11: LDL(address, 11); return;
  case 12: LDL(address, 12); return;
  case 13: LDL(address, 13); return;
  case 14: LDL(address, 14); return;
  default: assert(0); __builtin_unreachable();
  }
}

//! \brief Send load half word command at given address on the given output channel.
//! \param address address to send.
//! \param output must be an integer literal.
//!
//! Use \ref loki_load_half_word instead where possible.
#define LDHWU(address, output) {\
  asm (\
    "fetchr 0f\n"\
    "ldhwu.eop 0(%0) -> " #output "\n0:\n"\
    :\
    : "r" ((void *)address)\
    : "memory"\
  );\
}

//! Send a load half word memory operation on a given output channel.
static inline void loki_channel_load_half_word(
  const int channel, void const *address
) {
  switch (channel) {
  case 0: LDHWU(address, 0); return;
  case 1: LDHWU(address, 1); return;
  case 2: LDHWU(address, 2); return;
  case 3: LDHWU(address, 3); return;
  case 4: LDHWU(address, 4); return;
  case 5: LDHWU(address, 5); return;
  case 6: LDHWU(address, 6); return;
  case 7: LDHWU(address, 7); return;
  case 8: LDHWU(address, 8); return;
  case 9: LDHWU(address, 9); return;
  case 10: LDHWU(address, 10); return;
  case 11: LDHWU(address, 11); return;
  case 12: LDHWU(address, 12); return;
  case 13: LDHWU(address, 13); return;
  case 14: LDHWU(address, 14); return;
  default: assert(0); __builtin_unreachable();
  }
}

//! \brief Send load byte command at given address on the given output channel.
//! \param address address to send.
//! \param output must be an integer literal.
//!
//! Use \ref loki_load_byte instead where possible.
#define LDBU(address, output) {\
  asm (\
    "fetchr 0f\n"\
    "ldbu.eop 0(%0) -> " #output "\n0:\n"\
    :\
    : "r" ((void *)address)\
    : "memory"\
  );\
}

//! Send a load byte memory operation on a given output channel.
static inline void loki_channel_load_byte(
  const int channel, void const *address
) {
  switch (channel) {
  case 0: LDBU(address, 0); return;
  case 1: LDBU(address, 1); return;
  case 2: LDBU(address, 2); return;
  case 3: LDBU(address, 3); return;
  case 4: LDBU(address, 4); return;
  case 5: LDBU(address, 5); return;
  case 6: LDBU(address, 6); return;
  case 7: LDBU(address, 7); return;
  case 8: LDBU(address, 8); return;
  case 9: LDBU(address, 9); return;
  case 10: LDBU(address, 10); return;
  case 11: LDBU(address, 11); return;
  case 12: LDBU(address, 12); return;
  case 13: LDBU(address, 13); return;
  case 14: LDBU(address, 14); return;
  default: assert(0); __builtin_unreachable();
  }
}

//! Send a validate cache line memory operation on a given output channel.
static inline void loki_channel_validate_cache_line(const int channel, void *address) {
  switch (channel) {
  case 0: SENDCONFIG(address, SC_VALIDATE_LINE, 0); return;
  case 1: SENDCONFIG(address, SC_VALIDATE_LINE, 1); return;
  case 2: SENDCONFIG(address, SC_VALIDATE_LINE, 2); return;
  case 3: SENDCONFIG(address, SC_VALIDATE_LINE, 3); return;
  case 4: SENDCONFIG(address, SC_VALIDATE_LINE, 4); return;
  case 5: SENDCONFIG(address, SC_VALIDATE_LINE, 5); return;
  case 6: SENDCONFIG(address, SC_VALIDATE_LINE, 6); return;
  case 7: SENDCONFIG(address, SC_VALIDATE_LINE, 7); return;
  case 8: SENDCONFIG(address, SC_VALIDATE_LINE, 8); return;
  case 9: SENDCONFIG(address, SC_VALIDATE_LINE, 9); return;
  case 10: SENDCONFIG(address, SC_VALIDATE_LINE, 10); return;
  case 11: SENDCONFIG(address, SC_VALIDATE_LINE, 11); return;
  case 12: SENDCONFIG(address, SC_VALIDATE_LINE, 12); return;
  case 13: SENDCONFIG(address, SC_VALIDATE_LINE, 13); return;
  case 14: SENDCONFIG(address, SC_VALIDATE_LINE, 14); return;
  default: assert(0); __builtin_unreachable();
  }
}

//! Send a prefetch cache line memory operation on a given output channel.
static inline void loki_channel_prefetch_cache_line(
  const int channel, void const *address
) {
  switch (channel) {
  case 0: SENDCONFIG(address, SC_PREFETCH_LINE, 0); return;
  case 1: SENDCONFIG(address, SC_PREFETCH_LINE, 1); return;
  case 2: SENDCONFIG(address, SC_PREFETCH_LINE, 2); return;
  case 3: SENDCONFIG(address, SC_PREFETCH_LINE, 3); return;
  case 4: SENDCONFIG(address, SC_PREFETCH_LINE, 4); return;
  case 5: SENDCONFIG(address, SC_PREFETCH_LINE, 5); return;
  case 6: SENDCONFIG(address, SC_PREFETCH_LINE, 6); return;
  case 7: SENDCONFIG(address, SC_PREFETCH_LINE, 7); return;
  case 8: SENDCONFIG(address, SC_PREFETCH_LINE, 8); return;
  case 9: SENDCONFIG(address, SC_PREFETCH_LINE, 9); return;
  case 10: SENDCONFIG(address, SC_PREFETCH_LINE, 10); return;
  case 11: SENDCONFIG(address, SC_PREFETCH_LINE, 11); return;
  case 12: SENDCONFIG(address, SC_PREFETCH_LINE, 12); return;
  case 13: SENDCONFIG(address, SC_PREFETCH_LINE, 13); return;
  case 14: SENDCONFIG(address, SC_PREFETCH_LINE, 14); return;
  default: assert(0); __builtin_unreachable();
  }
}

//! Send a flush cache line memory operation on a given output channel.
static inline void loki_channel_flush_cache_line(
  const int channel, void const *address
) {
  switch (channel) {
  case 0: SENDCONFIG(address, SC_FLUSH_LINE, 0); return;
  case 1: SENDCONFIG(address, SC_FLUSH_LINE, 1); return;
  case 2: SENDCONFIG(address, SC_FLUSH_LINE, 2); return;
  case 3: SENDCONFIG(address, SC_FLUSH_LINE, 3); return;
  case 4: SENDCONFIG(address, SC_FLUSH_LINE, 4); return;
  case 5: SENDCONFIG(address, SC_FLUSH_LINE, 5); return;
  case 6: SENDCONFIG(address, SC_FLUSH_LINE, 6); return;
  case 7: SENDCONFIG(address, SC_FLUSH_LINE, 7); return;
  case 8: SENDCONFIG(address, SC_FLUSH_LINE, 8); return;
  case 9: SENDCONFIG(address, SC_FLUSH_LINE, 9); return;
  case 10: SENDCONFIG(address, SC_FLUSH_LINE, 10); return;
  case 11: SENDCONFIG(address, SC_FLUSH_LINE, 11); return;
  case 12: SENDCONFIG(address, SC_FLUSH_LINE, 12); return;
  case 13: SENDCONFIG(address, SC_FLUSH_LINE, 13); return;
  case 14: SENDCONFIG(address, SC_FLUSH_LINE, 14); return;
  default: assert(0); __builtin_unreachable();
  }
}

//! \brief Flush a data structure to the next level of memory hierarchy.
//!
//! Flush as many cache lines as are necessary to push the given data to the
//! next level of the memory hierarchy.
//!
//! This implementation also waits until the flush is complete, so is not
//! suitable for multiple quick flushes in succession.
static inline void loki_channel_flush_data(
  const int channel, void const *address, size_t size
) {
  char* cacheLine = (char*)((int)address & ~0x1f);
  char* endData = (char*)address + size;
  while (cacheLine < endData) {
    loki_channel_flush_cache_line(channel, cacheLine);
    cacheLine += 32;
  }
  
  // Load a value from each cache line to ensure that the flush has finished.
  cacheLine = (char*)((int)address & ~0x1f);  
  while (cacheLine < endData) {
    loki_channel_load_word(channel, cacheLine);
    cacheLine += 32;
    int tmp = loki_receive(2);
    
    // Include some empty code which uses tmp, so it doesn't get optimised away.
    asm ("" : : "r" (tmp) : "memory");
  }
}

//! Send a invalidate cache line memory operation on a given output channel.
static inline void loki_channel_invalidate_cache_line(
  const int channel, void *address
) {
  switch (channel) {
  case 0: SENDCONFIG(address, SC_INVALIDATE_LINE, 0); return;
  case 1: SENDCONFIG(address, SC_INVALIDATE_LINE, 1); return;
  case 2: SENDCONFIG(address, SC_INVALIDATE_LINE, 2); return;
  case 3: SENDCONFIG(address, SC_INVALIDATE_LINE, 3); return;
  case 4: SENDCONFIG(address, SC_INVALIDATE_LINE, 4); return;
  case 5: SENDCONFIG(address, SC_INVALIDATE_LINE, 5); return;
  case 6: SENDCONFIG(address, SC_INVALIDATE_LINE, 6); return;
  case 7: SENDCONFIG(address, SC_INVALIDATE_LINE, 7); return;
  case 8: SENDCONFIG(address, SC_INVALIDATE_LINE, 8); return;
  case 9: SENDCONFIG(address, SC_INVALIDATE_LINE, 9); return;
  case 10: SENDCONFIG(address, SC_INVALIDATE_LINE, 10); return;
  case 11: SENDCONFIG(address, SC_INVALIDATE_LINE, 11); return;
  case 12: SENDCONFIG(address, SC_INVALIDATE_LINE, 12); return;
  case 13: SENDCONFIG(address, SC_INVALIDATE_LINE, 13); return;
  case 14: SENDCONFIG(address, SC_INVALIDATE_LINE, 14); return;
  default: assert(0); __builtin_unreachable();
  }
}

//! \brief Invalidate a region of memory, forcing it to be fetched from the next
//! level of the memory hierarchy next time it is accessed.
//!
//! Invalidate as many cache lines as are necessary to capture the whole
//! requested region. Any other data which happens to share these lines will be
//! lost.
static inline void loki_channel_invalidate_data(
  const int channel, void const *address, size_t size
) {
  char* cacheLine = (char*)((int)address & ~0x1f);
  char* endData = (char*)address + size;
  while (cacheLine < endData) {
    loki_channel_invalidate_cache_line(channel, cacheLine);
    cacheLine += 32;
  }
}

//! Send a flush all lines memory operation on a given output channel.
static inline void loki_channel_flush_all_lines(
  const int channel, void const *address
) {
  switch (channel) {
  case 0: SENDCONFIG(address, SC_FLUSH_ALL_LINES, 0); return;
  case 1: SENDCONFIG(address, SC_FLUSH_ALL_LINES, 1); return;
  case 2: SENDCONFIG(address, SC_FLUSH_ALL_LINES, 2); return;
  case 3: SENDCONFIG(address, SC_FLUSH_ALL_LINES, 3); return;
  case 4: SENDCONFIG(address, SC_FLUSH_ALL_LINES, 4); return;
  case 5: SENDCONFIG(address, SC_FLUSH_ALL_LINES, 5); return;
  case 6: SENDCONFIG(address, SC_FLUSH_ALL_LINES, 6); return;
  case 7: SENDCONFIG(address, SC_FLUSH_ALL_LINES, 7); return;
  case 8: SENDCONFIG(address, SC_FLUSH_ALL_LINES, 8); return;
  case 9: SENDCONFIG(address, SC_FLUSH_ALL_LINES, 9); return;
  case 10: SENDCONFIG(address, SC_FLUSH_ALL_LINES, 10); return;
  case 11: SENDCONFIG(address, SC_FLUSH_ALL_LINES, 11); return;
  case 12: SENDCONFIG(address, SC_FLUSH_ALL_LINES, 12); return;
  case 13: SENDCONFIG(address, SC_FLUSH_ALL_LINES, 13); return;
  case 14: SENDCONFIG(address, SC_FLUSH_ALL_LINES, 14); return;
  default: assert(0); __builtin_unreachable();
  }
}

//! Send a invalidate all lines memory operation on a given output channel.
static inline void loki_channel_invalidate_all_lines(
  const int channel, void *address
) {
  switch (channel) {
  case 0: SENDCONFIG(address, SC_INVALIDATE_ALL_LINES, 0); return;
  case 1: SENDCONFIG(address, SC_INVALIDATE_ALL_LINES, 1); return;
  case 2: SENDCONFIG(address, SC_INVALIDATE_ALL_LINES, 2); return;
  case 3: SENDCONFIG(address, SC_INVALIDATE_ALL_LINES, 3); return;
  case 4: SENDCONFIG(address, SC_INVALIDATE_ALL_LINES, 4); return;
  case 5: SENDCONFIG(address, SC_INVALIDATE_ALL_LINES, 5); return;
  case 6: SENDCONFIG(address, SC_INVALIDATE_ALL_LINES, 6); return;
  case 7: SENDCONFIG(address, SC_INVALIDATE_ALL_LINES, 7); return;
  case 8: SENDCONFIG(address, SC_INVALIDATE_ALL_LINES, 8); return;
  case 9: SENDCONFIG(address, SC_INVALIDATE_ALL_LINES, 9); return;
  case 10: SENDCONFIG(address, SC_INVALIDATE_ALL_LINES, 10); return;
  case 11: SENDCONFIG(address, SC_INVALIDATE_ALL_LINES, 11); return;
  case 12: SENDCONFIG(address, SC_INVALIDATE_ALL_LINES, 12); return;
  case 13: SENDCONFIG(address, SC_INVALIDATE_ALL_LINES, 13); return;
  case 14: SENDCONFIG(address, SC_INVALIDATE_ALL_LINES, 14); return;
  default: assert(0); __builtin_unreachable();
  }
}

//! \brief Send store word command at given address on the given output channel.
//! \param variable value to send.
//! \param address address to send.
//! \param output must be an integer literal.
//!
//! Use \ref loki_store_word instead where possible.
#define STW(address, variable, output) {\
  asm (\
    "fetchr 0f\n"\
    "stw.eop %1, 0(%0) -> " #output "\n0:\n"\
    :\
    : "r" ((void *)address), "r" ((int)variable)\
    : "memory"\
  );\
}

//! Send a store word memory operation on a given output channel.
static inline void loki_channel_store_word(
  const int channel, void *address, int value
) {
  switch (channel) {
  case 0: STW(address, value, 0); return;
  case 1: STW(address, value, 1); return;
  case 2: STW(address, value, 2); return;
  case 3: STW(address, value, 3); return;
  case 4: STW(address, value, 4); return;
  case 5: STW(address, value, 5); return;
  case 6: STW(address, value, 6); return;
  case 7: STW(address, value, 7); return;
  case 8: STW(address, value, 8); return;
  case 9: STW(address, value, 9); return;
  case 10: STW(address, value, 10); return;
  case 11: STW(address, value, 11); return;
  case 12: STW(address, value, 12); return;
  case 13: STW(address, value, 13); return;
  case 14: STW(address, value, 14); return;
  default: assert(0); __builtin_unreachable();
  }
}

//! \brief Send store conditional command at given address on the given output channel.
//! \param variable value to send.
//! \param address address to send.
//! \param output must be an integer literal.
//!
//! Use \ref loki_store_conditional instead where possible.
#define STC(address, variable, output) {\
  asm (\
    "fetchr 0f\n"\
    "stc.eop %1, %0, 0 -> " #output "\n0:\n"\
    :\
    : "r" ((void *)address), "r" ((int)variable)\
    : "memory"\
  );\
}

//! Send a store conditional memory operation on a given output channel.
static inline void loki_channel_store_conditional(
  const int channel, void *address, int value
) {
  switch (channel) {
  case 0: STC(address, value, 0); return;
  case 1: STC(address, value, 1); return;
  case 2: STC(address, value, 2); return;
  case 3: STC(address, value, 3); return;
  case 4: STC(address, value, 4); return;
  case 5: STC(address, value, 5); return;
  case 6: STC(address, value, 6); return;
  case 7: STC(address, value, 7); return;
  case 8: STC(address, value, 8); return;
  case 9: STC(address, value, 9); return;
  case 10: STC(address, value, 10); return;
  case 11: STC(address, value, 11); return;
  case 12: STC(address, value, 12); return;
  case 13: STC(address, value, 13); return;
  case 14: STC(address, value, 14); return;
  default: assert(0); __builtin_unreachable();
  }
}

//! \brief Send store half word command at given address on the given output channel.
//! \param variable value to send.
//! \param address address to send.
//! \param output must be an integer literal.
//!
//! Use \ref loki_store_half_word instead where possible.
#define STHW(address, variable, output) {\
  asm (\
    "fetchr 0f\n"\
    "sthw.eop %1, 0(%0) -> " #output "\n0:\n"\
    :\
    : "r" ((void *)address), "r" ((int)variable)\
    : "memory"\
  );\
}

//! Send a store half word memory operation on a given output channel.
static inline void loki_channel_store_half_word(
  const int channel, void *address, int value
) {
  switch (channel) {
  case 0: STHW(address, value, 0); return;
  case 1: STHW(address, value, 1); return;
  case 2: STHW(address, value, 2); return;
  case 3: STHW(address, value, 3); return;
  case 4: STHW(address, value, 4); return;
  case 5: STHW(address, value, 5); return;
  case 6: STHW(address, value, 6); return;
  case 7: STHW(address, value, 7); return;
  case 8: STHW(address, value, 8); return;
  case 9: STHW(address, value, 9); return;
  case 10: STHW(address, value, 10); return;
  case 11: STHW(address, value, 11); return;
  case 12: STHW(address, value, 12); return;
  case 13: STHW(address, value, 13); return;
  case 14: STHW(address, value, 14); return;
  default: assert(0); __builtin_unreachable();
  }
}

//! \brief Send store byte command at given address on the given output channel.
//! \param variable value to send.
//! \param address address to send.
//! \param output must be an integer literal.
//!
//! Use \ref loki_store_byte instead where possible.
#define STB(address, variable, output) {\
  asm (\
    "fetchr 0f\n"\
    "stb.eop %1, 0(%0) -> " #output "\n0:\n"\
    :\
    : "r" ((void *)address), "r" ((int)variable)\
    : "memory"\
  );\
}

//! Send a store byte memory operation on a given output channel.
static inline void loki_channel_store_byte(
  const int channel, void *address, int value
) {
  switch (channel) {
  case 0: STB(address, value, 0); return;
  case 1: STB(address, value, 1); return;
  case 2: STB(address, value, 2); return;
  case 3: STB(address, value, 3); return;
  case 4: STB(address, value, 4); return;
  case 5: STB(address, value, 5); return;
  case 6: STB(address, value, 6); return;
  case 7: STB(address, value, 7); return;
  case 8: STB(address, value, 8); return;
  case 9: STB(address, value, 9); return;
  case 10: STB(address, value, 10); return;
  case 11: STB(address, value, 11); return;
  case 12: STB(address, value, 12); return;
  case 13: STB(address, value, 13); return;
  case 14: STB(address, value, 14); return;
  default: assert(0); __builtin_unreachable();
  }
}

//! Send a store cache line memory operation on a given output channel.
static inline void loki_channel_store_cache_line(
  const int channel, void *address,
  int value0, int value1, int value2, int value3,
  int value4, int value5, int value6, int value7
) {
  switch (channel) {
  case 0: SENDCONFIG9(address, SC_STORE_LINE, value0, value1, value2, value3, value4, value5, value6, value7, SC_PAYLOAD, 0); return;
  case 1: SENDCONFIG9(address, SC_STORE_LINE, value0, value1, value2, value3, value4, value5, value6, value7, SC_PAYLOAD, 1); return;
  case 2: SENDCONFIG9(address, SC_STORE_LINE, value0, value1, value2, value3, value4, value5, value6, value7, SC_PAYLOAD, 2); return;
  case 3: SENDCONFIG9(address, SC_STORE_LINE, value0, value1, value2, value3, value4, value5, value6, value7, SC_PAYLOAD, 3); return;
  case 4: SENDCONFIG9(address, SC_STORE_LINE, value0, value1, value2, value3, value4, value5, value6, value7, SC_PAYLOAD, 4); return;
  case 5: SENDCONFIG9(address, SC_STORE_LINE, value0, value1, value2, value3, value4, value5, value6, value7, SC_PAYLOAD, 5); return;
  case 6: SENDCONFIG9(address, SC_STORE_LINE, value0, value1, value2, value3, value4, value5, value6, value7, SC_PAYLOAD, 6); return;
  case 7: SENDCONFIG9(address, SC_STORE_LINE, value0, value1, value2, value3, value4, value5, value6, value7, SC_PAYLOAD, 7); return;
  case 8: SENDCONFIG9(address, SC_STORE_LINE, value0, value1, value2, value3, value4, value5, value6, value7, SC_PAYLOAD, 8); return;
  case 9: SENDCONFIG9(address, SC_STORE_LINE, value0, value1, value2, value3, value4, value5, value6, value7, SC_PAYLOAD, 9); return;
  case 10: SENDCONFIG9(address, SC_STORE_LINE, value0, value1, value2, value3, value4, value5, value6, value7, SC_PAYLOAD, 10); return;
  case 11: SENDCONFIG9(address, SC_STORE_LINE, value0, value1, value2, value3, value4, value5, value6, value7, SC_PAYLOAD, 11); return;
  case 12: SENDCONFIG9(address, SC_STORE_LINE, value0, value1, value2, value3, value4, value5, value6, value7, SC_PAYLOAD, 12); return;
  case 13: SENDCONFIG9(address, SC_STORE_LINE, value0, value1, value2, value3, value4, value5, value6, value7, SC_PAYLOAD, 13); return;
  case 14: SENDCONFIG9(address, SC_STORE_LINE, value0, value1, value2, value3, value4, value5, value6, value7, SC_PAYLOAD, 14); return;
  default: assert(0); __builtin_unreachable();
  }
}

//! Send a memset cache line memory operation on a given output channel.
static inline void loki_channel_memset_cache_line(
  const int channel, void *address, int value
) {
  switch (channel) {
  case 0: SENDCONFIG2(address, SC_MEMSET_LINE, value, SC_PAYLOAD_EOP, 0); return;
  case 1: SENDCONFIG2(address, SC_MEMSET_LINE, value, SC_PAYLOAD_EOP, 1); return;
  case 2: SENDCONFIG2(address, SC_MEMSET_LINE, value, SC_PAYLOAD_EOP, 2); return;
  case 3: SENDCONFIG2(address, SC_MEMSET_LINE, value, SC_PAYLOAD_EOP, 3); return;
  case 4: SENDCONFIG2(address, SC_MEMSET_LINE, value, SC_PAYLOAD_EOP, 4); return;
  case 5: SENDCONFIG2(address, SC_MEMSET_LINE, value, SC_PAYLOAD_EOP, 5); return;
  case 6: SENDCONFIG2(address, SC_MEMSET_LINE, value, SC_PAYLOAD_EOP, 6); return;
  case 7: SENDCONFIG2(address, SC_MEMSET_LINE, value, SC_PAYLOAD_EOP, 7); return;
  case 8: SENDCONFIG2(address, SC_MEMSET_LINE, value, SC_PAYLOAD_EOP, 8); return;
  case 9: SENDCONFIG2(address, SC_MEMSET_LINE, value, SC_PAYLOAD_EOP, 9); return;
  case 10: SENDCONFIG2(address, SC_MEMSET_LINE, value, SC_PAYLOAD_EOP, 10); return;
  case 11: SENDCONFIG2(address, SC_MEMSET_LINE, value, SC_PAYLOAD_EOP, 11); return;
  case 12: SENDCONFIG2(address, SC_MEMSET_LINE, value, SC_PAYLOAD_EOP, 12); return;
  case 13: SENDCONFIG2(address, SC_MEMSET_LINE, value, SC_PAYLOAD_EOP, 13); return;
  case 14: SENDCONFIG2(address, SC_MEMSET_LINE, value, SC_PAYLOAD_EOP, 14); return;
  default: assert(0); __builtin_unreachable();
  }
}

//! \brief Starting at `address`, set the following `size` words to `value`.
static inline void loki_channel_memset_words(
  const int channel, void* address, int value, size_t size
) {
  int* dataPtr = address;
  int* endData = dataPtr + size;
  
  // Special case if everything fits entirely within one cache line.
  if (((int)dataPtr & ~0x1f) == ((int)endData & ~0x1f)) {
    for ( ; dataPtr < endData; dataPtr++)
      loki_channel_store_word(channel, dataPtr, value);
  }
  else {
    // Store individual words up to the next cache line boundary.
    for ( ; ((int)dataPtr & 0x1f) != 0; dataPtr++)
      loki_channel_store_word(channel, dataPtr, value);
    
    // Set entire cache lines.
    for ( ; dataPtr + 8 < endData; dataPtr += 8)
      loki_channel_memset_cache_line(channel, dataPtr, value);
    
    // Store individual words for the final part of a cache line.
    for ( ; dataPtr < endData; dataPtr++)
      loki_channel_store_word(channel, dataPtr, value);
  }
}

//! Send a push cache line operation on the given output channel.
static inline void loki_channel_push_cache_line(
  const int channel, void *address,
  int value0, int value1, int value2, int value3,
  int value4, int value5, int value6, int value7
) {
  switch (channel) {
  case 0: SENDCONFIG9(address, SC_PUSH_LINE | SC_SKIP_L1, value0, value1, value2, value3, value4, value5, value6, value7, SC_PAYLOAD | SC_SKIP_L1, 0); return;
  case 1: SENDCONFIG9(address, SC_PUSH_LINE | SC_SKIP_L1, value0, value1, value2, value3, value4, value5, value6, value7, SC_PAYLOAD | SC_SKIP_L1, 1); return;
  case 2: SENDCONFIG9(address, SC_PUSH_LINE | SC_SKIP_L1, value0, value1, value2, value3, value4, value5, value6, value7, SC_PAYLOAD | SC_SKIP_L1, 2); return;
  case 3: SENDCONFIG9(address, SC_PUSH_LINE | SC_SKIP_L1, value0, value1, value2, value3, value4, value5, value6, value7, SC_PAYLOAD | SC_SKIP_L1, 3); return;
  case 4: SENDCONFIG9(address, SC_PUSH_LINE | SC_SKIP_L1, value0, value1, value2, value3, value4, value5, value6, value7, SC_PAYLOAD | SC_SKIP_L1, 4); return;
  case 5: SENDCONFIG9(address, SC_PUSH_LINE | SC_SKIP_L1, value0, value1, value2, value3, value4, value5, value6, value7, SC_PAYLOAD | SC_SKIP_L1, 5); return;
  case 6: SENDCONFIG9(address, SC_PUSH_LINE | SC_SKIP_L1, value0, value1, value2, value3, value4, value5, value6, value7, SC_PAYLOAD | SC_SKIP_L1, 6); return;
  case 7: SENDCONFIG9(address, SC_PUSH_LINE | SC_SKIP_L1, value0, value1, value2, value3, value4, value5, value6, value7, SC_PAYLOAD | SC_SKIP_L1, 7); return;
  case 8: SENDCONFIG9(address, SC_PUSH_LINE | SC_SKIP_L1, value0, value1, value2, value3, value4, value5, value6, value7, SC_PAYLOAD | SC_SKIP_L1, 8); return;
  case 9: SENDCONFIG9(address, SC_PUSH_LINE | SC_SKIP_L1, value0, value1, value2, value3, value4, value5, value6, value7, SC_PAYLOAD | SC_SKIP_L1, 9); return;
  case 10: SENDCONFIG9(address, SC_PUSH_LINE | SC_SKIP_L1, value0, value1, value2, value3, value4, value5, value6, value7, SC_PAYLOAD | SC_SKIP_L1, 10); return;
  case 11: SENDCONFIG9(address, SC_PUSH_LINE | SC_SKIP_L1, value0, value1, value2, value3, value4, value5, value6, value7, SC_PAYLOAD | SC_SKIP_L1, 11); return;
  case 12: SENDCONFIG9(address, SC_PUSH_LINE | SC_SKIP_L1, value0, value1, value2, value3, value4, value5, value6, value7, SC_PAYLOAD | SC_SKIP_L1, 12); return;
  case 13: SENDCONFIG9(address, SC_PUSH_LINE | SC_SKIP_L1, value0, value1, value2, value3, value4, value5, value6, value7, SC_PAYLOAD | SC_SKIP_L1, 13); return;
  case 14: SENDCONFIG9(address, SC_PUSH_LINE | SC_SKIP_L1, value0, value1, value2, value3, value4, value5, value6, value7, SC_PAYLOAD | SC_SKIP_L1, 14); return;
  default: assert(0); __builtin_unreachable();
  }
}

//! \brief Send load and add command at given address on the given output channel.
//! \param variable value to send.
//! \param address address to send.
//! \param output must be an integer literal.
//!
//! Use \ref loki_load_and_add instead where possible.
#define LDADD(address, variable, output) {\
  asm (\
    "fetchr 0f\n"\
    "ldadd.eop %1, %0, 0 -> " #output "\n0:\n"\
    :\
    : "r" ((void *)address), "r" ((int)variable)\
    : "memory"\
  );\
}

//! Send a load and add memory operation on a given output channel.
static inline void loki_channel_load_and_add(
  const int channel, void *address, int value
) {
  switch (channel) {
  case 0: LDADD(address, value, 0); return;
  case 1: LDADD(address, value, 1); return;
  case 2: LDADD(address, value, 2); return;
  case 3: LDADD(address, value, 3); return;
  case 4: LDADD(address, value, 4); return;
  case 5: LDADD(address, value, 5); return;
  case 6: LDADD(address, value, 6); return;
  case 7: LDADD(address, value, 7); return;
  case 8: LDADD(address, value, 8); return;
  case 9: LDADD(address, value, 9); return;
  case 10: LDADD(address, value, 10); return;
  case 11: LDADD(address, value, 11); return;
  case 12: LDADD(address, value, 12); return;
  case 13: LDADD(address, value, 13); return;
  case 14: LDADD(address, value, 14); return;
  default: assert(0); __builtin_unreachable();
  }
}

//! \brief Send load and or command at given address on the given output channel.
//! \param variable value to send.
//! \param address address to send.
//! \param output must be an integer literal.
//!
//! Use \ref loki_load_and_or instead where possible.
#define LDOR(address, variable, output) {\
  asm (\
    "fetchr 0f\n"\
    "ldor.eop %1, %0, 0 -> " #output "\n0:\n"\
    :\
    : "r" ((void *)address), "r" ((int)variable)\
    : "memory"\
  );\
}

//! Send a load and or memory operation on a given output channel.
static inline void loki_channel_load_and_or(
  const int channel, void *address, int value
) {
  switch (channel) {
  case 0: LDOR(address, value, 0); return;
  case 1: LDOR(address, value, 1); return;
  case 2: LDOR(address, value, 2); return;
  case 3: LDOR(address, value, 3); return;
  case 4: LDOR(address, value, 4); return;
  case 5: LDOR(address, value, 5); return;
  case 6: LDOR(address, value, 6); return;
  case 7: LDOR(address, value, 7); return;
  case 8: LDOR(address, value, 8); return;
  case 9: LDOR(address, value, 9); return;
  case 10: LDOR(address, value, 10); return;
  case 11: LDOR(address, value, 11); return;
  case 12: LDOR(address, value, 12); return;
  case 13: LDOR(address, value, 13); return;
  case 14: LDOR(address, value, 14); return;
  default: assert(0); __builtin_unreachable();
  }
}

//! \brief Send load and and command at given address on the given output channel.
//! \param variable value to send.
//! \param address address to send.
//! \param output must be an integer literal.
//!
//! Use \ref loki_load_and_and instead where possible.
#define LDAND(address, variable, output) {\
  asm (\
    "fetchr 0f\n"\
    "ldand.eop %1, %0, 0 -> " #output "\n0:\n"\
    :\
    : "r" ((void *)address), "r" ((int)variable)\
    : "memory"\
  );\
}

//! Send a load and and memory operation on a given output channel.
static inline void loki_channel_load_and_and(
  const int channel, void *address, int value
) {
  switch (channel) {
  case 0: LDAND(address, value, 0); return;
  case 1: LDAND(address, value, 1); return;
  case 2: LDAND(address, value, 2); return;
  case 3: LDAND(address, value, 3); return;
  case 4: LDAND(address, value, 4); return;
  case 5: LDAND(address, value, 5); return;
  case 6: LDAND(address, value, 6); return;
  case 7: LDAND(address, value, 7); return;
  case 8: LDAND(address, value, 8); return;
  case 9: LDAND(address, value, 9); return;
  case 10: LDAND(address, value, 10); return;
  case 11: LDAND(address, value, 11); return;
  case 12: LDAND(address, value, 12); return;
  case 13: LDAND(address, value, 13); return;
  case 14: LDAND(address, value, 14); return;
  default: assert(0); __builtin_unreachable();
  }
}

//! \brief Send load and xor command at given address on the given output channel.
//! \param variable value to send.
//! \param address address to send.
//! \param output must be an integer literal.
//!
//! Use \ref loki_load_and_xor instead where possible.
#define LDXOR(address, variable, output) {\
  asm (\
    "fetchr 0f\n"\
    "ldxor.eop %1, %0, 0 -> " #output "\n0:\n"\
    :\
    : "r" ((void *)address), "r" ((int)variable)\
    : "memory"\
  );\
}

//! Send a load and xor memory operation on a given output channel.
static inline void loki_channel_load_and_xor(
  const int channel, void *address, int value
) {
  switch (channel) {
  case 0: LDXOR(address, value, 0); return;
  case 1: LDXOR(address, value, 1); return;
  case 2: LDXOR(address, value, 2); return;
  case 3: LDXOR(address, value, 3); return;
  case 4: LDXOR(address, value, 4); return;
  case 5: LDXOR(address, value, 5); return;
  case 6: LDXOR(address, value, 6); return;
  case 7: LDXOR(address, value, 7); return;
  case 8: LDXOR(address, value, 8); return;
  case 9: LDXOR(address, value, 9); return;
  case 10: LDXOR(address, value, 10); return;
  case 11: LDXOR(address, value, 11); return;
  case 12: LDXOR(address, value, 12); return;
  case 13: LDXOR(address, value, 13); return;
  case 14: LDXOR(address, value, 14); return;
  default: assert(0); __builtin_unreachable();
  }
}

//! \brief Send exchange command at given address on the given output channel.
//! \param variable value to send.
//! \param address address to send.
//! \param output must be an integer literal.
//!
//! Use \ref loki_exchange instead where possible.
#define EXCHANGE(address, variable, output) {\
  asm (\
    "fetchr 0f\n"\
    "exchange.eop %1, %0, 0 -> " #output "\n0:\n"\
    :\
    : "r" ((void *)address), "r" ((int)variable)\
    : "memory"\
  );\
}

//! Send a exchange memory operation on a given output channel.
static inline void loki_channel_exchange(
  const int channel, void *address, int value
) {
  switch (channel) {
  case 0: EXCHANGE(address, value, 0); return;
  case 1: EXCHANGE(address, value, 1); return;
  case 2: EXCHANGE(address, value, 2); return;
  case 3: EXCHANGE(address, value, 3); return;
  case 4: EXCHANGE(address, value, 4); return;
  case 5: EXCHANGE(address, value, 5); return;
  case 6: EXCHANGE(address, value, 6); return;
  case 7: EXCHANGE(address, value, 7); return;
  case 8: EXCHANGE(address, value, 8); return;
  case 9: EXCHANGE(address, value, 9); return;
  case 10: EXCHANGE(address, value, 10); return;
  case 11: EXCHANGE(address, value, 11); return;
  case 12: EXCHANGE(address, value, 12); return;
  case 13: EXCHANGE(address, value, 13); return;
  case 14: EXCHANGE(address, value, 14); return;
  default: assert(0); __builtin_unreachable();
  }
}

//! Send a update directory entry memory operation on a given output channel.
static inline void loki_channel_update_directory_entry(
  const int channel, void *address, int value
) {
  switch (channel) {
  case 0: SENDCONFIG2(address, SC_UPDATE_DIRECTORY_ENTRY, value, SC_PAYLOAD, 0); return;
  case 1: SENDCONFIG2(address, SC_UPDATE_DIRECTORY_ENTRY, value, SC_PAYLOAD, 1); return;
  case 2: SENDCONFIG2(address, SC_UPDATE_DIRECTORY_ENTRY, value, SC_PAYLOAD, 2); return;
  case 3: SENDCONFIG2(address, SC_UPDATE_DIRECTORY_ENTRY, value, SC_PAYLOAD, 3); return;
  case 4: SENDCONFIG2(address, SC_UPDATE_DIRECTORY_ENTRY, value, SC_PAYLOAD, 4); return;
  case 5: SENDCONFIG2(address, SC_UPDATE_DIRECTORY_ENTRY, value, SC_PAYLOAD, 5); return;
  case 6: SENDCONFIG2(address, SC_UPDATE_DIRECTORY_ENTRY, value, SC_PAYLOAD, 6); return;
  case 7: SENDCONFIG2(address, SC_UPDATE_DIRECTORY_ENTRY, value, SC_PAYLOAD, 7); return;
  case 8: SENDCONFIG2(address, SC_UPDATE_DIRECTORY_ENTRY, value, SC_PAYLOAD, 8); return;
  case 9: SENDCONFIG2(address, SC_UPDATE_DIRECTORY_ENTRY, value, SC_PAYLOAD, 9); return;
  case 10: SENDCONFIG2(address, SC_UPDATE_DIRECTORY_ENTRY, value, SC_PAYLOAD, 10); return;
  case 11: SENDCONFIG2(address, SC_UPDATE_DIRECTORY_ENTRY, value, SC_PAYLOAD, 11); return;
  case 12: SENDCONFIG2(address, SC_UPDATE_DIRECTORY_ENTRY, value, SC_PAYLOAD, 12); return;
  case 13: SENDCONFIG2(address, SC_UPDATE_DIRECTORY_ENTRY, value, SC_PAYLOAD, 13); return;
  case 14: SENDCONFIG2(address, SC_UPDATE_DIRECTORY_ENTRY, value, SC_PAYLOAD, 14); return;
  default: assert(0); __builtin_unreachable();
  }
}

//! Send a update directory mask memory operation on a given output channel.
static inline void loki_channel_update_directory_mask(
  const int channel, void *address, int value
) {
  switch (channel) {
  case 0: SENDCONFIG2(address, SC_UPDATE_DIRECTORY_MASK, value, SC_PAYLOAD, 0); return;
  case 1: SENDCONFIG2(address, SC_UPDATE_DIRECTORY_MASK, value, SC_PAYLOAD, 1); return;
  case 2: SENDCONFIG2(address, SC_UPDATE_DIRECTORY_MASK, value, SC_PAYLOAD, 2); return;
  case 3: SENDCONFIG2(address, SC_UPDATE_DIRECTORY_MASK, value, SC_PAYLOAD, 3); return;
  case 4: SENDCONFIG2(address, SC_UPDATE_DIRECTORY_MASK, value, SC_PAYLOAD, 4); return;
  case 5: SENDCONFIG2(address, SC_UPDATE_DIRECTORY_MASK, value, SC_PAYLOAD, 5); return;
  case 6: SENDCONFIG2(address, SC_UPDATE_DIRECTORY_MASK, value, SC_PAYLOAD, 6); return;
  case 7: SENDCONFIG2(address, SC_UPDATE_DIRECTORY_MASK, value, SC_PAYLOAD, 7); return;
  case 8: SENDCONFIG2(address, SC_UPDATE_DIRECTORY_MASK, value, SC_PAYLOAD, 8); return;
  case 9: SENDCONFIG2(address, SC_UPDATE_DIRECTORY_MASK, value, SC_PAYLOAD, 9); return;
  case 10: SENDCONFIG2(address, SC_UPDATE_DIRECTORY_MASK, value, SC_PAYLOAD, 10); return;
  case 11: SENDCONFIG2(address, SC_UPDATE_DIRECTORY_MASK, value, SC_PAYLOAD, 11); return;
  case 12: SENDCONFIG2(address, SC_UPDATE_DIRECTORY_MASK, value, SC_PAYLOAD, 12); return;
  case 13: SENDCONFIG2(address, SC_UPDATE_DIRECTORY_MASK, value, SC_PAYLOAD, 13); return;
  case 14: SENDCONFIG2(address, SC_UPDATE_DIRECTORY_MASK, value, SC_PAYLOAD, 14); return;
  default: assert(0); __builtin_unreachable();
  }
}

//! \brief Send a channel acquire operation on a given output channel.
//! \param id The chanel to send on.
//! \param message The payload of the message.
static inline void loki_channel_acquire_ex(
    int const channel
  , unsigned int const message
) {
  switch (channel) {
  case 0: SENDCONFIG(message, SC_UNACQUIRED | SC_ALLOCATE | SC_EOP, 0); return;
  case 1: SENDCONFIG(message, SC_UNACQUIRED | SC_ALLOCATE | SC_EOP, 1); return;
  case 2: SENDCONFIG(message, SC_UNACQUIRED | SC_ALLOCATE | SC_EOP, 2); return;
  case 3: SENDCONFIG(message, SC_UNACQUIRED | SC_ALLOCATE | SC_EOP, 3); return;
  case 4: SENDCONFIG(message, SC_UNACQUIRED | SC_ALLOCATE | SC_EOP, 4); return;
  case 5: SENDCONFIG(message, SC_UNACQUIRED | SC_ALLOCATE | SC_EOP, 5); return;
  case 6: SENDCONFIG(message, SC_UNACQUIRED | SC_ALLOCATE | SC_EOP, 6); return;
  case 7: SENDCONFIG(message, SC_UNACQUIRED | SC_ALLOCATE | SC_EOP, 7); return;
  case 8: SENDCONFIG(message, SC_UNACQUIRED | SC_ALLOCATE | SC_EOP, 8); return;
  case 9: SENDCONFIG(message, SC_UNACQUIRED | SC_ALLOCATE | SC_EOP, 9); return;
  case 10: SENDCONFIG(message, SC_UNACQUIRED | SC_ALLOCATE | SC_EOP, 10); return;
  case 11: SENDCONFIG(message, SC_UNACQUIRED | SC_ALLOCATE | SC_EOP, 11); return;
  case 12: SENDCONFIG(message, SC_UNACQUIRED | SC_ALLOCATE | SC_EOP, 12); return;
  case 13: SENDCONFIG(message, SC_UNACQUIRED | SC_ALLOCATE | SC_EOP, 13); return;
  case 14: SENDCONFIG(message, SC_UNACQUIRED | SC_ALLOCATE | SC_EOP, 14); return;
  default: assert(0); __builtin_unreachable();
  }
}

//! \brief Send a channel aquisition message on the specified channel.
//! \param output must be an integer constant.
//!
//! Use functions instead of this macro where possible.

// The message records the source of the request, i.e:
// temp = (src_core) | (src_tile_y << 4) | (src_tile_x << 7) | (cmt_entry << 16)
//    the core and tile location are read from CREG 1
//    the top 16-bits is set to the channel name ("output")
//
// SEND_CONFIG out-of-band bits for core-to-core communications are
//       bit 2 : acquire
//       bit 1 : allocate
//       bit 0 : eop
//
// The out-of-band bits are set to 0x3 to request a channel is allocated.
//
#define CHANNEL_ACQUIRE(output) {\
  register int temp;\
  asm volatile (\
    "fetchr 0f\n"\
    "cregrdi %0, 1\n"\
    "lui %0, %1\n"\
    "sendconfig.eop %0, %2 -> %1\n0:\n"\
    : "+r"(temp)\
    : "n"(output), "n"(SC_UNACQUIRED | SC_ALLOCATE | SC_EOP)\
  );\
}

//! \brief Send a channel acquire operation on a given output channel.
//! \param id The chanel to send on.
static inline void loki_channel_acquire(int const channel) {
  switch (channel) {
  case 0: CHANNEL_ACQUIRE(0); return;
  case 1: CHANNEL_ACQUIRE(1); return;
  case 2: CHANNEL_ACQUIRE(2); return;
  case 3: CHANNEL_ACQUIRE(3); return;
  case 4: CHANNEL_ACQUIRE(4); return;
  case 5: CHANNEL_ACQUIRE(5); return;
  case 6: CHANNEL_ACQUIRE(6); return;
  case 7: CHANNEL_ACQUIRE(7); return;
  case 8: CHANNEL_ACQUIRE(8); return;
  case 9: CHANNEL_ACQUIRE(9); return;
  case 10: CHANNEL_ACQUIRE(10); return;
  case 11: CHANNEL_ACQUIRE(11); return;
  case 12: CHANNEL_ACQUIRE(12); return;
  case 13: CHANNEL_ACQUIRE(13); return;
  case 14: CHANNEL_ACQUIRE(14); return;
  default: assert(0); __builtin_unreachable();
  }
}

//! Send a channel release operation on a given output channel.
static inline void loki_channel_release(int const channel) {
  switch (channel) {
  case 0: SENDCONFIG(0, SC_ACQUIRED | SC_ALLOCATE | SC_EOP, 0); return;
  case 1: SENDCONFIG(0, SC_ACQUIRED | SC_ALLOCATE | SC_EOP, 1); return;
  case 2: SENDCONFIG(0, SC_ACQUIRED | SC_ALLOCATE | SC_EOP, 2); return;
  case 3: SENDCONFIG(0, SC_ACQUIRED | SC_ALLOCATE | SC_EOP, 3); return;
  case 4: SENDCONFIG(0, SC_ACQUIRED | SC_ALLOCATE | SC_EOP, 4); return;
  case 5: SENDCONFIG(0, SC_ACQUIRED | SC_ALLOCATE | SC_EOP, 5); return;
  case 6: SENDCONFIG(0, SC_ACQUIRED | SC_ALLOCATE | SC_EOP, 6); return;
  case 7: SENDCONFIG(0, SC_ACQUIRED | SC_ALLOCATE | SC_EOP, 7); return;
  case 8: SENDCONFIG(0, SC_ACQUIRED | SC_ALLOCATE | SC_EOP, 8); return;
  case 9: SENDCONFIG(0, SC_ACQUIRED | SC_ALLOCATE | SC_EOP, 9); return;
  case 10: SENDCONFIG(0, SC_ACQUIRED | SC_ALLOCATE | SC_EOP, 10); return;
  case 11: SENDCONFIG(0, SC_ACQUIRED | SC_ALLOCATE | SC_EOP, 11); return;
  case 12: SENDCONFIG(0, SC_ACQUIRED | SC_ALLOCATE | SC_EOP, 12); return;
  case 13: SENDCONFIG(0, SC_ACQUIRED | SC_ALLOCATE | SC_EOP, 13); return;
  case 14: SENDCONFIG(0, SC_ACQUIRED | SC_ALLOCATE | SC_EOP, 14); return;
  default: assert(0); __builtin_unreachable();
  }
}

//! \brief Wait on channel emptiness, a specified number of credits on a channel.
//! \param emptiness number of credits to wait for. Must be an integer constant.
//! \param output must be an integer constant.
//!
//! Use functions instead of this macro where possible.
#define WOCHE(emptiness, output) {\
  asm (\
    "fetchr 0f\n"\
    "woche.eop %0 -> %1\n0:\n"\
    :\
    : "n" (emptiness), "n"(output)\
    : "memory"\
  );\
}

//! Wait for the default number of credtis to return to the specified channel.
static inline void loki_channel_wait_empty(int const channel) {
  switch (loki_channel_default_credit_count(get_channel_map(channel))) {
  case DEFAULT_CREDIT_COUNT:
    switch (channel) {
    case 0: WOCHE(DEFAULT_CREDIT_COUNT, 0); return;
    case 1: WOCHE(DEFAULT_CREDIT_COUNT, 1); return;
    case 2: WOCHE(DEFAULT_CREDIT_COUNT, 2); return;
    case 3: WOCHE(DEFAULT_CREDIT_COUNT, 3); return;
    case 4: WOCHE(DEFAULT_CREDIT_COUNT, 4); return;
    case 5: WOCHE(DEFAULT_CREDIT_COUNT, 5); return;
    case 6: WOCHE(DEFAULT_CREDIT_COUNT, 6); return;
    case 7: WOCHE(DEFAULT_CREDIT_COUNT, 7); return;
    case 8: WOCHE(DEFAULT_CREDIT_COUNT, 8); return;
    case 9: WOCHE(DEFAULT_CREDIT_COUNT, 9); return;
    case 10: WOCHE(DEFAULT_CREDIT_COUNT, 10); return;
    case 11: WOCHE(DEFAULT_CREDIT_COUNT, 11); return;
    case 12: WOCHE(DEFAULT_CREDIT_COUNT, 12); return;
    case 13: WOCHE(DEFAULT_CREDIT_COUNT, 13); return;
    case 14: WOCHE(DEFAULT_CREDIT_COUNT, 14); return;
    default: assert(0); __builtin_unreachable();
    }
  case DEFAULT_IPK_FIFO_CREDIT_COUNT:
    switch (channel) {
    case 0: WOCHE(DEFAULT_IPK_FIFO_CREDIT_COUNT, 0); return;
    case 1: WOCHE(DEFAULT_IPK_FIFO_CREDIT_COUNT, 1); return;
    case 2: WOCHE(DEFAULT_IPK_FIFO_CREDIT_COUNT, 2); return;
    case 3: WOCHE(DEFAULT_IPK_FIFO_CREDIT_COUNT, 3); return;
    case 4: WOCHE(DEFAULT_IPK_FIFO_CREDIT_COUNT, 4); return;
    case 5: WOCHE(DEFAULT_IPK_FIFO_CREDIT_COUNT, 5); return;
    case 6: WOCHE(DEFAULT_IPK_FIFO_CREDIT_COUNT, 6); return;
    case 7: WOCHE(DEFAULT_IPK_FIFO_CREDIT_COUNT, 7); return;
    case 8: WOCHE(DEFAULT_IPK_FIFO_CREDIT_COUNT, 8); return;
    case 9: WOCHE(DEFAULT_IPK_FIFO_CREDIT_COUNT, 9); return;
    case 10: WOCHE(DEFAULT_IPK_FIFO_CREDIT_COUNT, 10); return;
    case 11: WOCHE(DEFAULT_IPK_FIFO_CREDIT_COUNT, 11); return;
    case 12: WOCHE(DEFAULT_IPK_FIFO_CREDIT_COUNT, 12); return;
    case 13: WOCHE(DEFAULT_IPK_FIFO_CREDIT_COUNT, 13); return;
    case 14: WOCHE(DEFAULT_IPK_FIFO_CREDIT_COUNT, 14); return;
    default: assert(0); __builtin_unreachable();
    }
  default: assert(0); __builtin_unreachable();
  }
}

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

  // Multicast channels are connected!
  if (1 != (c & 0x1)) return true;

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
//! \param allow_multicast Allow the use of a faster multicast channel where
//!        possible.
//!
//! This method overwrites the channel map table entry specified by id, and then
//! begins a connection procedure to that channel. The status of this connection
//! operation can be checked with \ref loki_connect_async_poll. Alternatively, a
//! core can sleep until the connection is completed with
//! \ref loki_connect_async_wait. A synchronous version of this operation is
//! available \ref loki_connect_ex.
//!
//! \remark In order to ensure deadlock freedom, the credit count is set to
//! \ref loki_default_credit_count(channel) for the connection.
//!
//! \remark If allow_multicast is set, the channel may be allocated as a multicast
//! as opposed to a core to core channel, which has different semantics but is
//! faster.
static inline void loki_connect_async_ex(
    int const id
  , tile_id_t const tile
  , enum Cores const core
  , enum Channels const channel
  , bool const allow_multicast
) {
  channel_t const c =
    allow_multicast && (tile == get_tile_id()) ?
      loki_mcast_address(
          single_core_bitmask(core)
        , channel
        , false
      )
    :
      loki_core_address(
          tile
        , core
        , channel
        , loki_default_credit_count(channel)
      );

  set_channel_map(id, c);

  loki_connect_async_poll(id);
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
  loki_connect_async_ex(id, tile, core, channel, false);
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
//! \param allow_multicast Allow the use of a faster multicast channel where
//!        possible.
//!
//! This method overwrites the channel map table entry specified by id, and then
//! performs a connection procedure to that channel. An asynchronous version of
//! this operation is available \ref loki_connect_async_ex.
//!
//! \remark In order to ensure deadlock freedom, the credit count is set to
//! \ref DEFAULT_CREDIT_COUNT for the connection.
//!
//! \remark If allow_multicast is set, the channel may be allocated as a multicast
//! as opposed to a core to core channel, which has different semantics but is
//! faster.
static inline void loki_connect_ex(
    int const id
  , tile_id_t const tile
  , enum Cores const core
  , enum Channels const channel
  , bool const allow_multicast
) {
  loki_connect_async_ex(id, tile, core, channel, allow_multicast);
  loki_connect_async_wait(id);
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
  loki_connect_ex(id, tile, core, channel, false);
}

//! \brief Disconnects the specified channel.
//! \param id The channel to disconnect.
//!
//! This operation releases the specified channel, allowing another core to
//! connect to it. It first blocks until the channel is empty, in order to
//! ensure that no credits are outstanding.
static inline void loki_disconnect(int const id) {
  channel_t const c = get_channel_map(id);

  // Multicast channels are not connected!
  if (1 != (c & 0x1)) return;

  loki_channel_wait_empty(id);
  loki_channel_release(id);
}

//! \brief Begins an asynchronous connection operation within a group.
//! \param id The channel to connect.
//! \param first_core_id The first core in the group.
//! \param index The index of the core in the group to connect to.
//! \param channel The destination channel to connect to.
//! \param allow_multicast Allow the use of a faster multicast channel where
//!        possible.
//!
//! This method overwrites the channel map table entry specified by id, and then
//! begins a connection procedure to that channel. The status of this connection
//! operation can be checked with \ref loki_connect_async_poll.
//! Alternatively, a core can sleep until the connection is completed with
//! \ref loki_connect_async_wait. A synchronous version of this operation
//! is available \ref loki_group_connect.
//!
//! \remark In order to ensure deadlock freedom, the credit count is set to
//! \ref loki_default_credit_count(channel) for the connection.
//!
//! \remark If allow_multicast is set, the channel may be allocated as a multicast
//! as opposed to a core to core channel, which has different semantics but is
//! faster.
static inline void loki_group_connect_async(
    int const id
  , core_id_t const first_core_id
  , unsigned int const index
  , enum Channels const channel
  , bool const allow_multicast
) {
  core_id_t const core_id = group_core_id(first_core_id, index);
  loki_connect_async_ex(
      id
    , get_unique_core_id_tile(core_id)
    , get_unique_core_id_core(core_id)
    , channel
    , allow_multicast
  );
}

//! \brief Connects to the specified destination within a group.
//! \param id The channel to connect.
//! \param first_core_id The first core in the group.
//! \param index The index of the core in the group to connect to.
//! \param channel The destination channel to connect to.
//! \param allow_multicast Allow the use of a faster multicast channel where
//!        possible.
//!
//!
//! This method overwrites the channel map table entry specified by id, and then
//! performs a connection procedure to that channel. An asynchronous version of
//! this operation is available \ref loki_group_connect_async.
//!
//! \remark In order to ensure deadlock freedom, the credit count is set to
//! \ref DEFAULT_CREDIT_COUNT for the connection.
//!
//! \remark If allow_multicast is set, the channel may be allocated as a multicast
//! as opposed to a core to core channel, which has different semantics but is
//! faster.
static inline void loki_group_connect(
    int const id
  , core_id_t const first_core_id
  , unsigned int const index
  , enum Channels const channel
  , bool const allow_multicast
) {
  loki_group_connect_async(id, first_core_id, index, channel, allow_multicast);
  loki_connect_async_wait(id);
}


#endif
