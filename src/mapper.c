#include "mapper.h"
#include <stdio.h>

uint8_t *mapper_test_read(Mapper *mapper, uint16_t addr) { return mapper->ram + addr; }

bool mapper_test_write(Mapper *mapper, uint16_t addr, uint8_t data) {
  mapper->ram[addr] = data;
  return true;
}

uint8_t *mapper_000_prg_read(Mapper *mapper, uint16_t addr) {
  if (addr >= 0x8000 && addr <= 0xFFFF) {
    uint16_t mask = mapper->prg_banks > 1 ? 0x7FFF : 0x3FFF;
    return mapper->prg_memory + (addr & mask);
  }

  return NULL;
}

bool mapper_000_prg_write(Mapper *mapper, uint16_t addr, uint8_t data) {
  if (addr >= 0x8000 && addr <= 0xFFFF) {
    mapper->prg_memory[addr & 0x7FFF] = data;
    return true;
  }

  return false;
}

uint8_t *mapper_000_chr_read(Mapper *mapper, uint16_t addr) {
  if (addr >= 0x0000 && addr <= 0x1FFF) {
    return mapper->chr_memory + addr;
  }

  return NULL;
}

bool mapper_000_chr_write(Mapper *mapper, uint16_t addr, uint8_t data) {
  if (addr >= 0x0000 && addr <= 0x1FFF) {
    if (mapper->chr_banks == 0) {
      mapper->chr_memory[addr] = data;
      return true;
    }
  }

  return false;
}

void mapper_init(Mapper *mapper, uint32_t mapper_id) {
  if (mapper_id == 0) {
    mapper->prg_read = mapper_000_prg_read;
    mapper->prg_write = mapper_000_prg_write;
  } else if (mapper_id == 0xffffffff) {
    mapper->prg_read = mapper_test_read;
    mapper->prg_write = mapper_test_write;
  } else {
    printf("Mapper %d not implemented\n", mapper_id);
    exit(1);
  }
}
