#ifndef __PPU_H__
#define __PPU_H__

#include "bus.h"
#include "common.h"

typedef struct {
  uint8_t nametable[2][1024];
  uint8_t palette[32];

  uint8_t frame_complete;
  uint16_t cycle;
  uint16_t scanline;

  int framebuffer[256 * 240];

  Bus *bus;
} PPU;

void ppu_init(PPU *ppu, Bus *bus);
void ppu_step(PPU *ppu);

#endif // __PPU_H__
