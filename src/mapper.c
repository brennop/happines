#include "mapper.h"
#include <stdio.h>

uint8_t *mapper_test_read(Mapper *mapper, uint16_t addr) {
  return mapper->ram + addr;
}

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

uint8_t mapper_000_mirror(Mapper *mapper) { return mapper->mirror_mode; }

/**
 * Mapper 1
 */

uint8_t *mapper_001_prg_read(Mapper *mapper, uint16_t addr) {
  if (addr >= 0x6000 && addr <= 0x7FFF) { // 8KB Ram bank
    return mapper->ram + (addr & 0x1FFF);
  }

  if (addr >= 0x8000 && addr <= 0xFFFF) { // 32KB Rom bank
    if (mapper->control & 0x08) {
      if (addr >= 0x8000 && addr <= 0xBFFF) {
        return mapper->prg_memory + (mapper->prg_bank_lo * 0x4000) +
               (addr & 0x3FFF);
      }

      if (addr >= 0xC000 && addr <= 0xFFFF) {
        return mapper->prg_memory + (mapper->prg_bank_hi * 0x4000) +
               (addr & 0x3FFF);
      }
    } else {
      return mapper->prg_memory + (addr & 0x7FFF) + mapper->prg_bank * 0x8000;
    }
  }

  return NULL;
}

const uint8_t mapper_001_mirror_lookup[4] = {
    MIRRORING_SINGLE_LOWER, MIRRORING_SINGLE_UPPER, MIRRORING_VERTICAL,
    MIRRORING_HORIZONTAL};

bool mapper_001_prg_write(Mapper *mapper, uint16_t addr, uint8_t data) {
  if (addr >= 0x6000 && addr <= 0x7FFF) { // 8KB Ram bank
    mapper->ram[addr & 0x1FFF] = data;

    return true;
  }

  if (addr >= 0x8000) {
    if (data & 0x80) { // reset
      mapper->load_register = 0;
      mapper->load_register_count = 0;
      mapper->control |= 0x0C;
    } else {
      mapper->load_register >>= 1;
      mapper->load_register |= (data & 0x01) << 4;
      mapper->load_register_count++;

      if (mapper->load_register_count == 5) {
        uint8_t reg = (addr >> 13) & 0x03; // bits 13 and 14

        if (reg == 0) {
          mapper->control = mapper->load_register & 0x1F;
          mapper->mirror_mode =
              mapper_001_mirror_lookup[mapper->control & 0x03];
        } else if (reg == 1) {
          if (mapper->control & 0x10) {
            mapper->chr_bank_lo = mapper->load_register & 0x1F;
          } else {
            mapper->chr_bank = mapper->load_register & 0x1E;
          }
        } else if (reg == 2) {
          if (mapper->control & 0x10) {
            mapper->chr_bank_hi = mapper->load_register & 0x1F;
          }
        } else if (reg == 3) {
          uint8_t prg_bank_mode = (mapper->control >> 2) & 0x03; // bits 2 and 3

          switch (prg_bank_mode) {
          case 0:
          case 1:
            mapper->prg_bank = (mapper->load_register & 0x0E) >> 1;
            break;
          case 2:
            mapper->prg_bank_lo = 0;
            mapper->prg_bank_hi = mapper->load_register & 0x0F;
            break;
          case 3:
            mapper->prg_bank_lo = mapper->load_register & 0x0F;
            mapper->prg_bank_hi = mapper->prg_banks - 1;
            break;
          }
        }

        mapper->load_register = 0;
        mapper->load_register_count = 0;
      }
    }
  }

  return false;
}

uint8_t *mapper_001_chr_read(Mapper *mapper, uint16_t addr) {
  if (addr >= 0x0000 && addr <= 0x1FFF) {
    if (mapper->control & 0x10) {
      if (addr >= 0x0000 && addr <= 0x0FFF) {
        return mapper->chr_memory + (mapper->chr_bank_lo * 0x1000) +
               (addr & 0x0FFF);
      }

      if (addr >= 0x1000 && addr <= 0x1FFF) {
        return mapper->chr_memory + (mapper->chr_bank_hi * 0x1000) +
               (addr & 0x0FFF);
      }
    } else {
      return mapper->chr_memory + (addr & 0x1FFF) + mapper->chr_bank * 0x2000;
    }
  }

  return NULL;
}

bool mapper_001_chr_write(Mapper *mapper, uint16_t addr, uint8_t data) {
  if (addr >= 0x0000 && addr <= 0x1FFF) {
    mapper->chr_memory[addr] = data;
    return true;
  }

  return false;
}

uint8_t mapper_001_mirror(Mapper *mapper) {
  return mapper->mirror_mode;
}

/**
 * Mapper 2
 */

uint8_t *mapper_002_prg_read(Mapper *mapper, uint16_t addr) {
  if (addr >= 0x8000 && addr <= 0xBFFF) {
    return mapper->prg_memory + (mapper->prg_bank_lo * 0x4000) +
           (addr & 0x3FFF);
  }

  if (addr >= 0xC000 && addr <= 0xFFFF) {
    return mapper->prg_memory + (mapper->prg_bank_hi * 0x4000) +
           (addr & 0x3FFF);
  }

  return NULL;
}

bool mapper_002_prg_write(Mapper *mapper, uint16_t addr, uint8_t data) {
  if (addr >= 0x8000 && addr <= 0xFFFF) {
    mapper->prg_bank_lo = data & 0x0F;
  }

  return false;
}

uint8_t *mapper_002_chr_read(Mapper *mapper, uint16_t addr) {
  if (addr >= 0x0000 && addr <= 0x1FFF) {
    return mapper->chr_memory + addr;
  }

  return NULL;
}

bool mapper_002_chr_write(Mapper *mapper, uint16_t addr, uint8_t data) {
  if (addr >= 0x0000 && addr <= 0x1FFF) {
    mapper->chr_memory[addr] = data;
    return true;
  }

  return false;
}

// end of mappers

void mapper_init(Mapper *mapper, uint32_t mapper_id, uint8_t mirror_mode) {
  mapper->mirror_mode = mirror_mode;

  if (mapper_id == 0) {
    mapper->prg_read = mapper_000_prg_read;
    mapper->prg_write = mapper_000_prg_write;
    mapper->chr_read = mapper_000_chr_read;
    mapper->chr_write = mapper_000_chr_write;
    mapper->mirror = mapper_000_mirror;
  } else if (mapper_id == 1) {
    mapper->prg_read = mapper_001_prg_read;
    mapper->prg_write = mapper_001_prg_write;
    mapper->chr_read = mapper_001_chr_read;
    mapper->chr_write = mapper_001_chr_write;
    mapper->mirror = mapper_001_mirror;

    mapper->control = 0x1C;
    mapper->prg_bank_lo = 0;
    mapper->prg_bank_hi = mapper->prg_banks - 1;
    mapper->prg_bank = 0;
  } else if (mapper_id == 2) {
    mapper->prg_read = mapper_002_prg_read;
    mapper->prg_write = mapper_002_prg_write;
    mapper->chr_read = mapper_002_chr_read;
    mapper->chr_write = mapper_002_chr_write;
    mapper->mirror = mapper_000_mirror;

    mapper->prg_bank_lo = 0;
    mapper->prg_bank_hi = mapper->prg_banks - 1;
  } else if (mapper_id == 0xffffffff) {
    mapper->prg_read = mapper_test_read;
    mapper->prg_write = mapper_test_write;
  } else {
    printf("Mapper %d not implemented\n", mapper_id);
    exit(1);
  }
}
