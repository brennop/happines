#include "emulator.h"

#include <stdio.h>

static void load_rom(Emulator *emulator, char *filename) {
  Mapper *mapper = &emulator->bus.mapper;
  FILE *file = fopen(filename, "rb");

  // read 16 byte header
  fread(&emulator->header, 1, 16, file);

  // skip trainer if present
  if (emulator->header.flags_6 & 0x04) {
    fseek(file, 512, SEEK_CUR);
  }

  /* initialize mapper variables */
  mapper->prg_rom_size = emulator->header.prg_rom_chunks * NES_PRG_ROM_CHUNK_SIZE;
  mapper->chr_rom_size = emulator->header.chr_rom_chunks * NES_CHR_ROM_CHUNK_SIZE;

  mapper->prg_banks = emulator->header.prg_rom_chunks;
  mapper->chr_banks = emulator->header.chr_rom_chunks;

  fread(mapper->prg_rom, 1, mapper->prg_rom_size, file);
  fread(mapper->chr_rom, 1, mapper->chr_rom_size, file);

  fclose(file);
}

void emulator_init(Emulator *emulator, char *filename) {
  load_rom(emulator, filename);

  // get mapper number
  uint32_t mapper_id = (emulator->header.flags_7 & 0xF0) | (emulator->header.flags_6 >> 4);

  bus_init(&emulator->bus);
  mapper_init(&emulator->bus.mapper, mapper_id);
}

void emulator_step(Emulator *emulator) {}
