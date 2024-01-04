#ifndef __EMULATOR_H__
#define __EMULATOR_H__

#include "bus.h"
#include "mapper.h"

#define NES_MAGIC_NUMBER "NES\x1a"
#define NES_PRG_ROM_CHUNK_SIZE 0x4000
#define NES_CHR_ROM_CHUNK_SIZE 0x2000

// iNES header
typedef struct {
  char magic[4];
  uint8_t prg_rom_chunks;
  uint8_t chr_rom_chunks;
  uint8_t flags_6;
  uint8_t flags_7;
  uint8_t flags_8;
  uint8_t flags_9;
  uint8_t flags_10;
  uint8_t zero[5];
} Header;

typedef struct {
  Bus bus;

  /**
   * Cartridge
   */
  Header header;

  int cycles;
} Emulator;

void emulator_init(Emulator *emulator, char *filename);
void emulator_step(Emulator *emulator);

#endif // __EMULATOR_H__


