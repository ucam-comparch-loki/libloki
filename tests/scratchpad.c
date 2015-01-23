#include <loki/scratchpad.h>
#include <stdio.h>
#include <stdlib.h>

// This program makes use of the scratchpad register file to store values locally,
// rather than having to read them from memory.

const char chars[] = {32, 33, 34, 35, 36, 37, 38, 39};
const short shorts[] = {40, 41, 42, 43, 44, 45, 46, 47};
const int ints[] = {48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63};

int main(int argc, char** argv) {
  // Store the three arrays to our register file.
  int answer[23];
  scratchpad_write_bytes(1 * 4, chars, sizeof(chars));
  memcpy(answer + 1, chars, sizeof(chars));
  scratchpad_write_bytes(3 * 4, shorts, sizeof(shorts));
  memcpy(answer + 3, shorts, sizeof(shorts));
  scratchpad_write_words(7, ints, sizeof(ints) / sizeof(ints[0]));
  memcpy(answer + 7, ints, sizeof(ints));

  // Also add an individual write, to show how that works.
  scratchpad_write(0, 100);
  answer[0] = 100;

  int addr;
  for (addr=0; addr<21; addr++) {
    if (scratchpad_read(addr) != answer[addr]) {
      fprintf(stderr, "Error: Address %d holds %d not %d\n", addr, scratchpad_read(addr), answer[addr]);
      exit(EXIT_FAILURE);
    }
  }
  exit(EXIT_SUCCESS);
}
