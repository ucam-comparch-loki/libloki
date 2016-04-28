/*! \file alloc.h
 * \brief Loki wrappers for malloc, free, etc. */

#ifndef LOKI_ALLOC_H_
#define LOKI_ALLOC_H_

//! \brief Make a previously-allocated memory block available for further
//! allocations.
void loki_free(void* ptr);

//! \brief Allocate a block of memory.
//!
//! This version differs from the standard malloc in that it ensures that 
//! no two allocations can ever share a cache line, reducing the risk of false
//! sharing in Loki's incoherent memory system.
void* loki_malloc(size_t size);

//! \brief Allocate and zero-initialise an array.
void* loki_calloc (size_t num, size_t size);

//! \brief Change the size of the memory block pointed to by `ptr`.
void* loki_realloc (void* ptr, size_t size);

#endif
