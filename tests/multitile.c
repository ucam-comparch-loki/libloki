#include <loki/lokilib.h>

int main(int argc, char** argv) {

  int address = loki_core_address(1, 0, 2);
  SET_CHANNEL_MAP(2, address);

  //start_debug_output();
  SEND(1000, 2);

}
