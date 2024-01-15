#ifndef __PPU_H__
#define __PPU_H__

#include "bus.h"
#include "common.h"
#include "mapper.h"

typedef struct {
  uint8_t y;
  uint8_t tile_id;
  uint8_t attributes;
  uint8_t x;
} Sprite;

typedef union {
  struct {
    uint16_t coarse_x : 5;
    uint16_t coarse_y : 5;
    uint16_t nametable_x : 1;
    uint16_t nametable_y : 1;
    uint16_t fine_y : 3;
    uint16_t unused : 1;
  };

  uint16_t reg;
} PPUAddress;

struct PPU {
  uint8_t raw_nametable[2 * 1024];
  uint8_t palette[32];

  uint32_t framebuffer[256 * 240];
  uint32_t nametable[2][256 * 240];
  uint32_t pattern_table[2][128 * 128];

  uint8_t frame_complete;
  uint16_t cycle;
  int scanline;
  uint8_t nmi;

  // Registers
  union {
    struct {
      uint8_t unused : 5;
      uint8_t sprite_overflow : 1;
      uint8_t sprite_zero_hit : 1;
      uint8_t vertical_blank : 1;
    };

    uint8_t reg;
  } status;

  union {
    struct {
      uint8_t grayscale : 1;
      uint8_t render_background_left : 1;
      uint8_t render_sprites_left : 1;
      uint8_t render_background : 1;
      uint8_t render_sprites : 1;
      uint8_t enhance_red : 1;
      uint8_t enhance_green : 1;
      uint8_t enhance_blue : 1;
    };

    uint8_t reg;
  } mask;

  union {
    struct {
      uint8_t nametable_x : 1;
      uint8_t nametable_y : 1;
      uint8_t increment_mode : 1;
      uint8_t pattern_sprite : 1;
      uint8_t pattern_background : 1;
      uint8_t sprite_size : 1;
      uint8_t slave_mode : 1; // unused
      uint8_t enable_nmi : 1;
    };

    uint8_t reg;
  } control;

  uint8_t address_latch;
  uint8_t ppu_data_buffer;

  PPUAddress vram_addr;
  PPUAddress tram_addr;
  uint8_t fine_x;

  uint8_t bg_next_tile_id;
  uint8_t bg_next_tile_attrib;
  uint8_t bg_next_tile_lsb;
  uint8_t bg_next_tile_msb;

  uint16_t bg_shifter_pattern_lo;
  uint16_t bg_shifter_pattern_hi;
  uint16_t bg_shifter_attrib_lo;
  uint16_t bg_shifter_attrib_hi;

  uint8_t oam_addr;
  Sprite oam[64];

  Sprite sprite_scanline[8];
  uint8_t sprite_count;
  uint8_t sprite_shifter_pattern_lo[8];
  uint8_t sprite_shifter_pattern_hi[8];

  Mapper *mapper;
};

void ppu_init(PPU *ppu, Mapper *mapper, uint8_t mirroring);
void ppu_step(PPU *ppu);
uint8_t ppu_control_read(PPU *ppu, uint16_t addr, bool readonly);
void ppu_control_write(PPU *ppu, uint16_t addr, uint8_t data);
uint32_t *ppu_get_pattern_table(PPU *ppu, uint8_t i, uint8_t palette);

#endif // __PPU_H__
