#include "emulator.h"

void emulator_init(Emulator *emulator, char *filename) {
  bus_init(&emulator->bus);
}
