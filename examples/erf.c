#include "../lokilib.h"
#include <stdio.h>
#include <stdlib.h>

// This program makes use of the extended register file to store values locally,
// rather than having to read them from memory.

char chars[] = {32, 33, 34, 35, 36, 37, 38, 39};
short shorts[] = {40, 41, 42, 43, 44, 45, 46, 47};
int ints[] = {48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63};

int main(int argc, char** argv) {
  // Store the three arrays to our register file.
  erf_block_store_chars(chars, 8, 32);
  erf_block_store_shorts(shorts, 8, 40);
  erf_block_store_ints(ints, 16, 48);
  
  // Also add an individual write, to show how that works.
  erf_write(31, 100);
  
  int reg;
  for (reg=31; reg<64; reg++) {
    printf("Register %d holds %d\n", reg, erf_read(reg));
  }
}
