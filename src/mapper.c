#include "mapper.h"
#include <stdio.h>

uint8_t *mapper_test_read(Mapper *mapper, uint16_t addr) { return mapper->ram + addr; }

bool mapper_test_write(Mapper *mapper, uint16_t addr, uint8_t data) {
  mapper->ram[addr] = data;
  return true;
}

uint8_t *mapper_000_read(Mapper *mapper, uint16_t addr) {
  if (addr >= 0x8000 && addr <= 0xFFFF) {
    uint16_t mask = mapper->prg_banks > 1 ? 0x7FFF : 0x3FFF;
    return mapper->prg_rom + (addr & mask);
  }

  return NULL;
}

bool mapper_000_write(Mapper *mapper, uint16_t addr, uint8_t data) {
  if (addr >= 0x8000 && addr <= 0xFFFF) {
    mapper->prg_rom[addr & 0x7FFF] = data;
    return true;
  }

  return false;
}

void mapper_init(Mapper *mapper, uint32_t mapper_id) {
  if (mapper_id == 0) {
    mapper->mapper_read = mapper_000_read;
    mapper->mapper_write = mapper_000_write;
  } else if (mapper_id == 0xffffffff) {
    mapper->mapper_read = mapper_test_read;
    mapper->mapper_write = mapper_test_write;
  } else {
    printf("Mapper %d not implemented\n", mapper_id);
    exit(1);
  }
}
