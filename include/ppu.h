#ifndef __PPU_H__
#define __PPU_H__

#include "bus.h"
#include "common.h"

struct PPU {
  uint8_t nametable[2][1024];
  uint8_t palette[32];

  uint8_t frame_complete;
  uint16_t cycle;
  uint16_t scanline;

  int framebuffer[256 * 240];

  uint32_t pattern_table[2][128 * 128];

  Bus *bus;
};

void ppu_init(PPU *ppu, Bus *bus);
void ppu_step(PPU *ppu);
uint8_t ppu_control_read(PPU *ppu, uint16_t addr, bool readonly);
void ppu_control_write(PPU *ppu, uint16_t addr, uint8_t data);
uint32_t *ppu_get_pattern_table(PPU *ppu, int i);

#endif // __PPU_H__
