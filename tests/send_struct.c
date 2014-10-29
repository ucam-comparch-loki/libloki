#include "../lokilib.h"
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

void core2() {
  data* d = malloc(sizeof(data));
  
  RECEIVE_STRUCT(d, sizeof(data), 7);
  
  printf("Core2: %d %d %d %d %s\n", d->word1, d->word2, d->short1, d->short2, d->string);

  exit(0);
}

void core1() {

  data* d = malloc(sizeof(data));
  
  d->word1 = 1;
  d->word2 = 2;
  d->short1 = 3;
  d->short2 = 4;
  d->string = "test"; // on stack - will this work?
  
  printf("Core1: %d %d %d %d %s\n", d->word1, d->word2, d->short1, d->short2, d->string);
  
  // start core 2
  loki_remote_execute(&core2, 1);

  int address = loki_core_address(0, 1, 7);
  SET_CHANNEL_MAP(10, address);
  SEND_STRUCT(d, sizeof(data), 10);
  
  loki_sleep();
}

int main(int argc, char** argv) {
  int addr0 = loki_mem_address(CH_IPK_CACHE, 0, 8, 0, GROUPSIZE_8, LINESIZE_32);
  int addr1 = loki_mem_address(CH_REGISTER_2, 0, 8, 0, GROUPSIZE_8, LINESIZE_32);  
  int config0 = loki_mem_config(ASSOCIATIVITY_1, LINESIZE_32, CACHE, GROUPSIZE_8);
  
  init_config* config = malloc(sizeof(init_config));
  config->cores = 2;
  config->stack_size = 0x12000;
  config->inst_mem = addr0;
  config->data_mem = addr1;
  config->mem_config = config0;
  config->config_func = NULL;//&loki_sleep;

  loki_init(config);
  
  core1();

}
