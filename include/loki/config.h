/*! \file channel_io.h
 * \brief Functions to send and receive messages on channels. */

#ifndef LOKI_CONFIG_H_
#define LOKI_CONFIG_H_

#include <assert.h>
#include <loki/channels.h>
#include <loki/types.h>

// TODO: formats of configuration messages.

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

#endif
