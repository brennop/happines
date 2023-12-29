#include "bus.h"
#include "emulator.h"

int main(int argc, char **argv) {
  Emulator emulator;

  emulator_init(&emulator, argv[1]);

  return 0;
}
