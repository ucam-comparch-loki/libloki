/*! \file control_registers.h
 * \brief Functions to access the control regsiters. */

#ifndef LOKI_CONTROL_REGISTERS_H_
#define LOKI_CONTROL_REGISTERS_H_

#include <assert.h>
#include <loki/types.h>

//! Control registers available in each core.
enum ControlRegisters {
  CR_CPU_LOCATION   =  1, //!< Tile ID and Core ID for this core.
  CR_COUNT0_CONFIG  =  4, //!< Configuration of the COUNT0 register.
  CR_COUNT1_CONFIG  =  5, //!< Configuration of the COUNT1 register.
  CR_COUNT0         =  6, //!< Free running counter.
  CR_COMPARE0       =  7, //!< Raise interrupt when COUNT0 == COMPARE0.
  CR_COUNT1         =  8, //!< Free running counter.
  CR_COMPARE1       =  9, //!< Raise interrupt when COUNT1 == COMPARE1.
  CR_CR10           = 10, //!< General purpose register.
  CR_CR11           = 11, //!< General purpose register.
  CR_CR12           = 12, //!< General purpose register.
  CR_CR13           = 13  //!< General purpose register.
};

//! Configurations to be stored in the count configuration registers, which
//! control the behaviour of the count registers.
enum CountConfig {
  CC_DISABLE            = 0,  //!< Counting disabled.
  CC_COUNT_CYCLES       = 1,  //!< Counter increments with each passing clock cycle.
  CC_COUNT_INSTRUCTIONS = 3,  //!< Counter increments with each instruction executed.
};

//! \brief Return the value in a control register.
//! \param variable must be an lvalue.
//! \param id must be an integer literal.
//!
//! Use \ref get_control_register instead where possible.
#define GET_CONTROL_REGISTER(variable, id) { \
  asm volatile ( \
    "fetchr 0f\n" \
    "cregrdi.eop %0, " #id "\n0:\n" \
    : "=r"(variable) \
    : \
    : \
  );\
}

//! Return the value in a control register.
static inline unsigned long get_control_register(enum ControlRegisters id) {
  unsigned long value;
  switch (id) {
  case CR_CPU_LOCATION:   GET_CONTROL_REGISTER(value,  1); return value;
  case CR_COUNT0_CONFIG:  GET_CONTROL_REGISTER(value,  4); return value;
  case CR_COUNT1_CONFIG:  GET_CONTROL_REGISTER(value,  5); return value;
  case CR_COUNT0:         GET_CONTROL_REGISTER(value,  6); return value;
  case CR_COMPARE0:       GET_CONTROL_REGISTER(value,  7); return value;
  case CR_COUNT1:         GET_CONTROL_REGISTER(value,  8); return value;
  case CR_COMPARE1:       GET_CONTROL_REGISTER(value,  9); return value;
  case CR_CR10:           GET_CONTROL_REGISTER(value, 10); return value;
  case CR_CR11:           GET_CONTROL_REGISTER(value, 11); return value;
  case CR_CR12:           GET_CONTROL_REGISTER(value, 12); return value;
  case CR_CR13:           GET_CONTROL_REGISTER(value, 13); return value;
  default:                assert(0);                       return 0xdeadbeef;
  }
}

//! \brief Set a control register.
//! \param variable value to set.
//! \param id must be an integer literal.
//!
//! Use \ref set_control_register instead where possible.
#define SET_CONTROL_REGISTER(variable, id) { \
  asm volatile ( \
    "fetchr 0f\n" \
    "cregwri.eop %0, " #id "\n0:\n" \
    : \
    : "r"(variable) \
    : \
  );\
}

//! Set a control register.
static inline void set_control_register(enum ControlRegisters id, unsigned long value) {
  switch (id) {
  case CR_CPU_LOCATION:   SET_CONTROL_REGISTER(value,  1); return;
  case CR_COUNT0_CONFIG:  SET_CONTROL_REGISTER(value,  4); return;
  case CR_COUNT1_CONFIG:  SET_CONTROL_REGISTER(value,  5); return;
  case CR_COUNT0:         SET_CONTROL_REGISTER(value,  6); return;
  case CR_COMPARE0:       SET_CONTROL_REGISTER(value,  7); return;
  case CR_COUNT1:         SET_CONTROL_REGISTER(value,  8); return;
  case CR_COMPARE1:       SET_CONTROL_REGISTER(value,  9); return;
  case CR_CR10:           SET_CONTROL_REGISTER(value, 10); return;
  case CR_CR11:           SET_CONTROL_REGISTER(value, 11); return;
  case CR_CR12:           SET_CONTROL_REGISTER(value, 12); return;
  case CR_CR13:           SET_CONTROL_REGISTER(value, 13); return;
  default:                assert(0);                       return;
  }
}

//! Configure COUNT0 to count clock cycles.
static inline void start_counting_cycles() {
  set_control_register(CR_COUNT0_CONFIG, CC_COUNT_CYCLES);
}

//! Configure COUNT0 to stop counting clock cycles.
static inline void stop_counting_cycles() {
  set_control_register(CR_COUNT0_CONFIG, CC_DISABLE);
}

//! Return the number of clock cycles which have passed since the first call to
//! this function. Assumes that COUNT0 is dedicated to counting cycles.
static inline unsigned long get_cycle_count() {
  // If the counter is not currently counting, start it now.
  start_counting_cycles();
  return get_control_register(CR_COUNT0);
}

//! Configure COUNT1 to count instructions executed.
static inline void start_counting_instructions() {
  set_control_register(CR_COUNT1_CONFIG, CC_COUNT_INSTRUCTIONS);
}

//! Configure COUNT1 to stop counting instructions executed.
static inline void stop_counting_instructions() {
  set_control_register(CR_COUNT1_CONFIG, CC_DISABLE);
}

//! Return the number of instructions executed on this core since the first call
//! to this function. Assumes that COUNT1 is dedicated to counting instructions.
static inline unsigned long get_instruction_count() {
  // If the counter is not currently counting, start it now.
  start_counting_instructions();
  return get_control_register(CR_COUNT1);
}

#endif
