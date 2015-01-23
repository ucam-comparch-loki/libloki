#include <loki/channels.h>
#include <loki/init.h>
#include <loki/spawn.h>
#include <stdio.h>
#include <stdlib.h>

int remote0(void) {
  return 0x0ddc0de;
}

int remote2 (int a, int b) {
  return a + b;
}

int remote5 (int a, int b, int c, int d, int e) {
  return a + b * c - d * e;
}

int main (int argc, char** argv) {

  // ~300 cycles
  loki_init_default(2, 0);

  int a=4, b=6, c=8, d=10, e=12;
  int result;

  // We want the result returned to this core (core 0). Send result to input 7.
  int address = loki_core_address(0, 0, 7);

  // Execute the remote function.
  loki_spawn(&remote0, address, 0 /* num args */);

  result = loki_receive(7);

  if (result != 0x0ddc0de) {
    fprintf(stderr, "remote0 returned incorrect result (%d != %d)\n", result, 0x0ddc0de);
    exit(EXIT_FAILURE);
  }

  // Execute the remote function.
  loki_spawn(&remote2, address, 2 /*num args*/, a, b);

  result = loki_receive(7);

  if (result != a+b) {
    fprintf(stderr, "remote2 returned incorrect result (%d != %d + %d)\n", result, a, b);
    exit(EXIT_FAILURE);
  }

  // Execute the remote function.
  loki_spawn(&remote5, address, 5 /*num args*/, a, b, c, d, e);

  result = loki_receive(7);

  if (result != a+b*c-d*e) {
    fprintf(stderr, "remote5 returned incorrect result (%d != %d + %d * %d - %d * %d)\n", result, a, b, c, d, e);
    exit(EXIT_FAILURE);
  }

  exit(EXIT_SUCCESS);

}
