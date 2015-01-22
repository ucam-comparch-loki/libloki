#include <loki/lokilib.h>
#include <stdio.h>
#include <stdlib.h>

struct data_ {
  int word1;
  int word2;
  short short1;
  short short2;
  char* string;
};
typedef struct data_ data;
data *d;

void core1() {
  data* e = malloc(sizeof(data));

  RECEIVE_STRUCT(e, sizeof(data), 7);
  if (memcmp(d, e, sizeof(data)) != 0) {
    fprintf(stderr, "SEND_STRUCT/RECEIVE_STRUCT mismatch.\n");
    exit(EXIT_FAILURE);
  }

  loki_receive_words((int *)e, sizeof(data) / 4, 7);
  if (memcmp(d, e, sizeof(data)) != 0) {
    fprintf(stderr, "loki_send_words/loki_receive_words mismatch.\n");
    exit(EXIT_FAILURE);
  }

  loki_receive_data((char *)e + 1, 13, 7);
  if (memcmp((char *)d + 1, (char *)e + 1, 13) != 0) {
    fprintf(stderr, "loki_send_data/loki_receive_data mismatch.\n");
    exit(EXIT_FAILURE);
  }

  exit(EXIT_SUCCESS);
}

void core0() {

  d = malloc(sizeof(data));

  d->word1 = 1;
  d->word2 = 2;
  d->short1 = 3;
  d->short2 = 4;
  d->string = "test";

  // start core 1
  loki_remote_execute(&core1, 1);

  channel_t address = loki_core_address(0, 1, CH_REGISTER_7);
  set_channel_map(10, address);
  SEND_STRUCT(d, sizeof(data), 10);
  loki_send_words((const int *)d, sizeof(data) / 4, 10);
  loki_send_data((char *)d + 1, 13, 10);

  loki_sleep();
}

int main(int argc, char** argv) {
  loki_init_default(2, 0);

  core0();
}
