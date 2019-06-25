/*! \file scratchpad.h
 * \brief Functions to access the core's local scratchpad.
 *
 * Each core has its own private scratchpad which it is responsible for
 * maintaining. The contents are completely software-managed: data may be
 * modified at any time, and state is not automatically pushed back to main
 * memory.
 *
 * The local scratchpad is intended to improve performance and energy efficiency
 * in some common use cases. To achieve this, its access is very restricted. It
 * *may* be more efficient to use a memory bank in scratchpad mode in the
 * following cases:
 *  * Accessing data which is not word-aligned.
 *  * Accessing data which requires a non-trivial addressing mode (e.g. base +
 * offset).
 */

#ifndef LOKI_SCRATCHPAD_H_
#define LOKI_SCRATCHPAD_H_

#include <assert.h>
#include <loki/types.h>
#include <string.h>

//! \brief Read the word in the scratchpad at the given address. The scratchpad is
//! word-addressed.
static inline int scratchpad_read(unsigned int address) {
  assert(address < SCRATCHPAD_NUM_WORDS);
  int result;
  asm volatile (
    "fetchr 0f\n"
    "scratchrd.eop %0, %1\n0:\n"
    : "=r" (result)
    : "r" (address)
    :
  );
  return result;
}

//! \brief Write the given word into the scratchpad at the given address. The
//! scratchpad is word-addressed.
static inline void scratchpad_write(unsigned int address, int value) {
  assert(address < SCRATCHPAD_NUM_WORDS);
  asm volatile (
    "fetchr 0f\n"
    "scratchwr.eop %1, %0\n0:\n"
    :
    : "r" (address), "r" (value)
    :
  );
}

//! \brief Read multiple words from the core's local scratchpad. The scratchpad
//! is word-addressed.
//! \param data Pointer to the buffer
//! \param address The position in the core's local scratchpad file to read the first value
//! \param len The number of words to read
//!
//! The order of arguments for these methods is designed to mimic memcpy.
static inline void scratchpad_read_words(int *data, unsigned int address, size_t len) {
  size_t i;
  for (i = 0; i != len; i++) {
    data[i] = scratchpad_read(address + i);
  }
}

//! \brief Read multiple data values from the core's local scratchpad. The
//! scratchpad is byte-addressed.
//! \param data Pointer to the buffer
//! \param address The position in bytes in the core's local scratchpad file to read the first value
//! \param len The number of bytes to read
//!
//! Note that the address parameter is in bytes!
//!
//! The order of arguments for these methods is designed to mimic memcpy.
static inline void scratchpad_read_bytes(void *data, unsigned int address, size_t len) {
  if (((int)data & 3) || (len & 3) || (address & 3)) {
    size_t words_touched = (len + 3) / 4 + ((address & 3) + 3) / 4;
    int buffer[words_touched];
    scratchpad_read_words(buffer, address / 4, words_touched);
    memcpy(data, (const char *)buffer + (address & 3), len);
  } else {
    scratchpad_read_words((int *)data, address / 4, len / 4);
  }
}

//! \brief Store multiple words in the core's local scratchpad. The scratchpad
//! is word-addressed.
//! \param address The position in the core's local scratchpad file to store the first value
//! \param data Pointer to the data
//! \param len The number of words to store
//!
//! The order of arguments for these methods is designed to mimic memcpy.
static inline void scratchpad_write_words(unsigned int address, const int *data, size_t len) {
  size_t i;
  for (i = 0; i != len; i++) {
    scratchpad_write(address + i, data[i]);
  }
}

//! \brief Store multiple data values in the core's local scratchpad. The
//! scratchpad is byte-addressed.
//! \param address The position in bytes in the core's local scratchpad file to store the first value
//! \param data Pointer to the data
//! \param len The number of bytes to store
//!
//! Note that the address parameter is in bytes!
//!
//! The order of arguments for these methods is designed to mimic memcpy.
static inline void scratchpad_write_bytes(unsigned int address, const void *data, size_t len) {
  if (((int)data & 3) || (len & 3) || (address & 3)) {
    size_t words_touched = (len + (address & 3) + 3) / 4;
    int buffer[words_touched];
    if (address & 3) {
      buffer[0] = scratchpad_read(address / 4);
    }
    if (len & 3) {
      buffer[words_touched - 1] = scratchpad_read(address / 4 + words_touched - 1);
    }
    memcpy((char *)buffer + (address & 3), data, len);
    scratchpad_write_words(address / 4, buffer, words_touched);
  } else {
    scratchpad_write_words(address / 4, (const int *)data, len / 4);
  }
}

#endif
