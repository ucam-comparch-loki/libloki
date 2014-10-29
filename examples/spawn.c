#include "../lokilib.h"
#include <stdio.h>

int remote (int a, int b) {
  return a + b;
}

int main (int argc, char** argv) {

  // ~300 cycles
  init_cores(2);

  int a=4, b=6;
  
  // We want the result returned to this core (core 0). Send result to input 7.
  int address = loki_core_address(0, 0, 7);
  
  // Execute the remote function.
  // ~250 cycles
  loki_spawn(&remote, address, 2 /*num args*/, a, b);
  
  int result;
  RECEIVE(result, 7);

  // ~18k cycles!
  printf("%d + %d = %d\n", a, b, result);
  
  return (result == a+b);

}
