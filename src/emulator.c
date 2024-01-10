#include "emulator.h"

#include <stdio.h>
#include <unistd.h>

static void load_rom(Emulator *emulator, char *filename) {
  FILE *file = fopen(filename, "rb");

  // read 16 byte header
  fread(&emulator->header, 1, 16, file);

  // skip trainer if present
  if (emulator->header[6] & 0x04) {
    fseek(file, 512, SEEK_CUR);
  }

  /* initialize mapper variables */
  emulator->mapper.prg_rom_size = emulator->header[4] * NES_PRG_ROM_CHUNK_SIZE;
  emulator->mapper.chr_rom_size = emulator->header[5] * NES_CHR_ROM_CHUNK_SIZE;

  emulator->mapper.prg_banks = emulator->header[4];
  emulator->mapper.chr_banks = emulator->header[5];

  fread(emulator->mapper.prg_memory, 1, emulator->mapper.prg_rom_size, file);
  fread(emulator->mapper.chr_memory, 1, emulator->mapper.chr_rom_size, file);

  fclose(file);
}

void emulator_init(Emulator *emulator, char *filename) {
  load_rom(emulator, filename);

  // get mapper number
  uint32_t mapper_id = (emulator->header[7] & 0xF0) | (emulator->header[6] >> 4);

  mapper_init(&emulator->mapper, mapper_id);
  bus_init(&emulator->bus, &emulator->mapper, &emulator->ppu);
  cpu_init(&emulator->cpu, &emulator->bus);
  ppu_init(&emulator->ppu, &emulator->mapper);
}

void emulator_step(Emulator *emulator) {
  while (emulator->ppu.frame_complete == false) {
    ppu_step(&emulator->ppu);
    if (emulator->cycles % 3 == 0) {
      cpu_step(&emulator->cpu);
    }

    emulator->cycles++;
  }

  emulator->ppu.frame_complete = false;
}
