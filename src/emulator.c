#include "emulator.h"

#include <stdio.h>

#include <time.h>

static void load_rom(Emulator *emulator, char *filename) {
  Mapper *mapper = &emulator->bus.mapper;
  FILE *file = fopen(filename, "rb");

  // read 16 byte header
  fread(&emulator->header, 1, 16, file);

  // skip trainer if present
  if (emulator->header[6] & 0x04) {
    fseek(file, 512, SEEK_CUR);
  }

  /* initialize mapper variables */
  mapper->prg_rom_size = emulator->header[4] * NES_PRG_ROM_CHUNK_SIZE;
  mapper->chr_rom_size = emulator->header[5] * NES_CHR_ROM_CHUNK_SIZE;

  mapper->prg_banks = emulator->header[4];
  mapper->chr_banks = emulator->header[5];

  fread(mapper->prg_memory, 1, mapper->prg_rom_size, file);
  fread(mapper->chr_memory, 1, mapper->chr_rom_size, file);

  fclose(file);
}

void emulator_init(Emulator *emulator, char *filename) {
  load_rom(emulator, filename);

  // get mapper number
  uint32_t mapper_id = (emulator->header[7] & 0xF0) | (emulator->header[6] >> 4);

  bus_init(&emulator->bus);
  mapper_init(&emulator->bus.mapper, mapper_id);
  cpu_init(&emulator->cpu, &emulator->bus);
  ppu_init(&emulator->ppu, &emulator->bus);
}

void emulator_step(Emulator *emulator) {
  while (emulator->ppu.frame_complete == false) {
    ppu_step(&emulator->ppu);
    if (emulator->cycles % 3 == 0) {
      cpu_step(&emulator->cpu);
    }

    emulator->cycles++;

    // sleep for 10ms
    nanosleep((const struct timespec[]){{0, 10000000L}}, NULL);
  }

  emulator->ppu.frame_complete = false;
}
