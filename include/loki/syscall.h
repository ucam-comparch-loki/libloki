/*! \file syscall.h
 * \brief Functions for using the syscall interface. */

#ifndef LOKI_SYSCALL_H_
#define LOKI_SYSCALL_H_

#include <loki/types.h>

//! \brief Execute a system call with the given immediate opcode.
//! \param opcode must be an integer literal.
//!
//! Use a specific syscall function instead where possible.
#define SYS_CALL(opcode) asm volatile (\
  "fetchr 0f\n"\
  "syscall.eop " #opcode "\n0:\n"\
)

#endif
