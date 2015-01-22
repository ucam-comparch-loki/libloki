/*! \file scratchpad.h
 * \brief Functions to access the core's local scratchpad. */

#ifndef LOKI_SCRATCHPAD_H_
#define LOKI_SCRATCHPAD_H_

#include <assert.h>
#include <string.h>

//! Number of words in each core's scratchpad.
#define SCRATCHPAD_NUM_WORDS 256

//! Read the word in the scratchpad at the given address.
static inline int scratchpad_read(unsigned int address) {
  assert(address < SCRATCHPAD_NUM_WORDS);
  int result;
  asm volatile (
    "scratchrd %0, %1\n"
    "fetchr.eop 0f\n0:\n"
    : "=r" (result)
    : "r" (address)
    :
  );
  return result;
}

//! Write the given word into the scratchpad at the given address.
static inline void scratchpad_write(unsigned int address, int value) {
  assert(address < SCRATCHPAD_NUM_WORDS);
  asm volatile (
    "scratchwr %0, %1\n"
    "fetchr.eop 0f\n0:\n"
    :
    : "r" (address), "r" (value)
    :
  );
}

//! \brief Read multiple words from the core's local scratchpad.
//! \param data pointer to the buffer
//! \param len the number of words to read
//! \param address the position in the core's local scratchpad file to read the first value
static inline void scratchpad_read_words(int *data, size_t len, unsigned int address) {
  size_t i;
  for (i = 0; i != len; i++) {
    data[i] = scratchpad_read(address + i);
  }
}

//! \brief Read multiple data values from the core's local scratchpad.
//! \param data pointer to the buffer
//! \param len the number of bytes to read
//! \param address the position in bytes in the core's local scratchpad file to read the first value
//!
//! Note that the address parameter is in bytes!
static inline void scratchpad_read_bytes(void *data, size_t len, unsigned int address) {
  if (((int)data & 3) || (len & 3) || (address & 3)) {
    size_t words_touched = (len + 3) / 4 + ((address & 3) + 3) / 4;
    int buffer[words_touched];
    scratchpad_read_words(buffer, words_touched, address / 4);
    memcpy(data, (const char *)buffer + (address & 3), len);
  } else {
    scratchpad_read_words((int *)data, len / 4, address / 4);
  }
}

//! \brief Store multiple words in the core's local scratchpad.
//! \param data pointer to the data
//! \param len the number of words to store
//! \param address the position in the core's local scratchpad file to store the first value
static inline void scratchpad_write_words(const int *data, size_t len, unsigned int address) {
  size_t i;
  for (i = 0; i != len; i++) {
    scratchpad_write(address + i, data[i]);
  }
}

//! \brief Store multiple data values in the core's local scratchpad.
//! \param data pointer to the data
//! \param len the number of bytes to store
//! \param address the position in bytes in the core's local scratchpad file to store the first value
//!
//! Note that the address parameter is in bytes!
static inline void scratchpad_write_bytes(const void *data, size_t len, unsigned int address) {
  if (((int)data & 3) || (len & 3) || (address & 3)) {
    size_t words_touched = (len + 3) / 4 + ((address & 3) + 3) / 4;
    int buffer[words_touched];
    if (address & 3) {
      buffer[0] = scratchpad_read(address / 4);
    }
    if (len & 3) {
      buffer[words_touched - 1] = scratchpad_read((address + len - 1) / 4);
    }
    memcpy((char *)buffer + (address & 3), data, len);
    scratchpad_write_words(buffer, words_touched, address / 4);
  } else {
    scratchpad_write_words((const int *)data, len / 4, address / 4);
  }
}

#endif
