#ifndef __EMULATOR_H__
#define __EMULATOR_H__

#include "bus.h"

typedef struct {
  Bus bus;

} Emulator;

void emulator_init(Emulator *emulator, char *filename);
void emulator_step(Emulator *emulator);
void emulator_update_timers(Emulator *emulator, int cycles);

#endif // __EMULATOR_H__


