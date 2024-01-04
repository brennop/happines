#ifndef __MAPPER_H__
#define __MAPPER_H__

#include "common.h"

typedef struct Mapper {
  uint8_t *(*mapper_read)(struct Mapper *, uint16_t);
  bool (*mapper_write)(struct Mapper *, uint16_t, uint8_t);

  // TODO: only for 6502 tests
  uint8_t ram[0x10000];

  uint8_t prg_rom_size;
  uint8_t prg_banks;
  uint8_t prg_rom[0x4000 * 64];

  uint8_t chr_banks;
  uint8_t chr_rom_size;
  uint8_t chr_rom[0x2000 * 64];
} Mapper;

void mapper_init(Mapper *mapper, uint32_t mapper_id);

#endif //__MAPPER_H__
