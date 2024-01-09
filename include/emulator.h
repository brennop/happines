#ifndef __EMULATOR_H__
#define __EMULATOR_H__

#include "bus.h"
#include "cpu.h"
#include "ppu.h"
#include "mapper.h"

#define NES_MAGIC_NUMBER "NES\x1a"
#define NES_PRG_ROM_CHUNK_SIZE 0x4000
#define NES_CHR_ROM_CHUNK_SIZE 0x2000

typedef struct {
  Bus bus;
  CPU cpu;
  PPU ppu;
  Mapper mapper;

  /**
   * Cartridge
   */
  uint8_t header[16];

  int cycles;
} Emulator;

void emulator_init(Emulator *emulator, char *filename);
void emulator_step(Emulator *emulator);

#endif // __EMULATOR_H__


