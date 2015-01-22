/*! \file barrier.h
 * \brief Memory barrier function. */

#ifndef LOKI_BARRIER_H_
#define LOKI_BARRIER_H_

//! \brief The compiler cannot move loads and stores from one side of a barrier to the
//! other.
static inline void barrier() {
  asm volatile ("" : : : "memory");
}

#endif
