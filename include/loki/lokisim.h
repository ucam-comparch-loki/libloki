/*! \file lokisim.h
 * \brief Functions specific to the lokisim simulator. */

#ifndef LOKI_LOKISIM_H_
#define LOKI_LOKISIM_H_

#include <loki/syscall.h>
#include <loki/types.h>

//! Start recording energy-consuming events
static inline void start_energy_log()        {SYS_CALL(0x20);}
//! Stop recording energy-consuming events
static inline void stop_energy_log()         {SYS_CALL(0x21);}
//! Print lots of information to stdout
static inline void start_debug_output()      {SYS_CALL(0x22);}
//! Stop printing debug information
static inline void stop_debug_output()       {SYS_CALL(0x23);}
//! Print address of every instruction executed
static inline void start_instruction_trace() {SYS_CALL(0x24);}
//! Stop printing instruction addresses
static inline void stop_instruction_trace()  {SYS_CALL(0x25);}

//! \brief Get the current cycle number
//!
//! \deprecated TODO
static inline uint current_cycle() {
  uint val;
  asm (
    "fetchr 0f\n"
    "syscall 0x30\n"
    "or.eop %0, r12, r0\n0:\n" // Assuming less than 2^32 cycles, otherwise need r11 too
    : "=r" (val)
    :
    : "r11", "r12"
  );
  return val;
}

#endif
