/*! \file channel_io.h
 * \brief Functions to send and receive messages on channels. */

#ifndef LOKI_CHANNEL_IO_H_
#define LOKI_CHANNEL_IO_H_

#include <assert.h>
#include <loki/channels.h>
#include <loki/types.h>
#include <string.h>

//! \brief Send the value of the named variable on the given output channel.
//! \param variable value to send.
//! \param output must be an integer literal.
//!
//! Use \ref loki_send instead where possible.
#define SEND(variable, output) {\
  asm (\
    "addu r0, %0, r0 -> " #output "\n"\
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
  default: assert(0); return;
  }
}

//! \brief Receive a value from a particular input and store it in the named variable.
//! \param variable must be an lvalue.
//! \param input must be an integer literal.
//!
//! Use \ref loki_receive instead where possible.
#define RECEIVE(variable, input) {\
  asm volatile (\
    "addu %0, r" #input ", r0\n"\
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
  default: assert(0); return 0xdeadbeef;
  }
}

//! \brief Send an entire struct to another core.
//! \param struct_ptr structure to send.
//! \param bytes number of bytes.
//! \param output must be an integer literal.
//!
//! Use \ref loki_send_data instead where possible.
// TODO: I can't get this to work properly without hard-coded registers.
#define SEND_STRUCT(struct_ptr, bytes, output) {\
  asm volatile (\
    "or r12, %0, r0\n"\
    "addu r11, %0, %1\n"\
    "ldw 0(r12) -> 1\n"\
    "addui r12, r12, 4\n"\
    "setlt.p r0, r12, r11\n"\
    "or r0, r2, r0 -> " #output "\n"\
    "ifp?ibjmp -16\n"\
    : \
    : "r" (struct_ptr), "r" (bytes)\
    : "r11", "r12"\
  );\
}

//! \brief Send an entire array to another core.
//! \param data structure to send. Must be 4 byte aligned.
//! \param size number of words.
//! \param output channel to send on.
static inline void loki_send_words(const int *data, size_t size, int output) {
  size_t i;
  for (i = 0; i != size; i++) {
    loki_send(data[i], output);
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
    loki_send_words(buffer, size, output);
  } else {
    loki_send_words((const int *)data, size / 4, output);
  }
}

//! \brief Receive an entire struct over the network.
//! \param struct_ptr destination buffer.
//! \param bytes number of bytes.
//! \param input must be an integer literal.
//!
//! Use \ref loki_receive_data instead where possible.
#define RECEIVE_STRUCT(struct_ptr, bytes, input) {\
  int end_of_struct;\
  void* struct_ptr_copy;\
  asm volatile (\
    "addu %0, %2, %3\n"\
    "or %1, %2, r0\n"\
    "stw r" #input ", 0(%1) -> 1\n"\
    "addui %1, %1, 4\n"\
    "setlt.p r0, %1, %0\n"\
    "ifp?ibjmp -12\n"\
    : "=r" (end_of_struct), "=r" (struct_ptr_copy)\
    : "r" (struct_ptr), "r" (bytes)\
    :\
  );\
}

//! \brief Receive an entire array from another core.
//! \param data buffer to fill. Must be 4 byte aligned.
//! \param size number of words.
//! \param input channel to receive on.
static inline void loki_receive_words(int *data, size_t size, enum Channels input) {
  size_t i;
  for (i = 0; i != size; i++) {
    data[i] = loki_receive(input);
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
    loki_receive_words(buffer, size, input);
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
    "rmtnxipk -> " #output "\n"\
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
  default: assert(0); return;
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

//! \brief Wait for input on any register-mapped input channel, and return the value
//! which arrived.
static inline int receive_any_input() {
  // r18 must be used for irdr in verilog implementation.
  int data;
  asm volatile (
    "selch r18, 0xFFFFFFFF\n"   // get the channel on which data first arrives
    "irdr %0, r18\n"            // get the data from the channel
    : "=r" (data)
    :
    : "r18"
  );

  return data;
}
#endif
