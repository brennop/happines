#ifndef __MAPPER_H__
#define __MAPPER_H__

#include "common.h"

typedef enum {
  MIRRORING_HORIZONTAL,
  MIRRORING_VERTICAL,
  MIRRORING_SINGLE_LOWER,
  MIRRORING_SINGLE_UPPER,
  MIRRORING_FOUR_SCREEN
} MirrorMode;

typedef struct Mapper {
  uint8_t *(*prg_read)(struct Mapper *, uint16_t);
  bool (*prg_write)(struct Mapper *, uint16_t, uint8_t);
  // vram
  uint8_t *(*chr_read)(struct Mapper *, uint16_t);
  bool (*chr_write)(struct Mapper *, uint16_t, uint8_t);
  uint8_t (*mirror)(struct Mapper *);
  void (*scanline)(struct Mapper *);

  uint8_t ram[0x10000];

  uint32_t prg_rom_size;
  uint8_t prg_banks;
  uint8_t prg_memory[0x4000 * 64];

  uint32_t chr_rom_size;
  uint8_t chr_banks;
  uint8_t chr_memory[0x2000 * 64];

  uint8_t mirror_mode;

  uint8_t control;
  uint8_t load_register;
  uint8_t load_register_count;

  uint8_t prg_bank;
  uint8_t prg_bank_lo;
  uint8_t prg_bank_hi;

  uint8_t chr_bank;
  uint8_t chr_bank_lo;
  uint8_t chr_bank_hi;

  uint32_t registers[8];
  uint32_t chr_offsets[8];
  uint32_t prg_offsets[4];

  uint8_t irq_enabled;
  uint8_t irq_active;
  uint16_t irq_counter;
  uint16_t irq_reload;

  uint8_t target_register;
  uint8_t prg_bank_mode;
  uint8_t chr_inversion;
} Mapper;

void mapper_init(Mapper *mapper, uint32_t mapper_id, uint8_t mirror_mode);

#endif //__MAPPER_H__
