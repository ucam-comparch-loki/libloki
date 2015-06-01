/*! \file channel_io.h
 * \brief Functions to send and receive messages on channels. */

#ifndef LOKI_CONFIG_H_
#define LOKI_CONFIG_H_

#include <assert.h>
#include <loki/channels.h>
#include <loki/types.h>

// TODO: formats of configuration messages.

//! "Configure" opcode + end of packet marker.
#define LOKI_MEMORY_CONFIGURATION 61

//! \brief Send the value of the named variable on the given output channel,
//! with the supplied metadata.
//! \param variable value to send.
//! \param metadata out-of-band information to send.
//! \param output must be an integer literal.
#define SEND_CONFIG(variable, metadata, output) {\
  asm (\
    "fetchr 0f\n"\
    "sendconfig.eop %0, " #metadata " -> " #output "\n0:\n"\
    :\
    : "r" ((int)variable)\
    :\
  );\
}

//! Generate a memory configuration command. Send to the first bank of a memory
//! group using loki_configure_memory.
static inline int loki_mem_configuration(const enum MemConfigAssociativity log2Assoc,
                                         const enum MemConfigLineSize      log2LineSize,
                                         const enum MemConfigType          isCache,
                                         const enum MemConfigGroupSize     log2GroupSize) {
  return (log2Assoc << 20) | (log2LineSize << 16) | (isCache << 8) | (log2GroupSize);
}

//! Send a configuration message to a memory bank.
static inline void loki_configure_memory(const int configuration,
                                         const enum Channels channel) {
  switch (channel) {
  case 0: SEND_CONFIG(configuration, LOKI_MEMORY_CONFIGURATION, 0); return;
  case 1: SEND_CONFIG(configuration, LOKI_MEMORY_CONFIGURATION, 1); return;
  case 2: SEND_CONFIG(configuration, LOKI_MEMORY_CONFIGURATION, 2); return;
  case 3: SEND_CONFIG(configuration, LOKI_MEMORY_CONFIGURATION, 3); return;
  case 4: SEND_CONFIG(configuration, LOKI_MEMORY_CONFIGURATION, 4); return;
  case 5: SEND_CONFIG(configuration, LOKI_MEMORY_CONFIGURATION, 5); return;
  case 6: SEND_CONFIG(configuration, LOKI_MEMORY_CONFIGURATION, 6); return;
  case 7: SEND_CONFIG(configuration, LOKI_MEMORY_CONFIGURATION, 7); return;
  case 8: SEND_CONFIG(configuration, LOKI_MEMORY_CONFIGURATION, 8); return;
  case 9: SEND_CONFIG(configuration, LOKI_MEMORY_CONFIGURATION, 9); return;
  case 10: SEND_CONFIG(configuration, LOKI_MEMORY_CONFIGURATION, 10); return;
  case 11: SEND_CONFIG(configuration, LOKI_MEMORY_CONFIGURATION, 11); return;
  case 12: SEND_CONFIG(configuration, LOKI_MEMORY_CONFIGURATION, 12); return;
  case 13: SEND_CONFIG(configuration, LOKI_MEMORY_CONFIGURATION, 13); return;
  case 14: SEND_CONFIG(configuration, LOKI_MEMORY_CONFIGURATION, 14); return;
  default: assert(0); return;
  }
}

#endif
