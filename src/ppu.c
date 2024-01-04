#include "ppu.h"

void ppu_init(PPU *ppu, Bus *bus) {
  ppu->bus = bus;
}

void ppu_step(PPU *ppu) {
  ppu->cycle++;

  if (ppu->cycle >= 341) {
    ppu->cycle = 0;
    ppu->scanline++;

    if (ppu->scanline == 261) {
      ppu->scanline = -1;
      ppu->frame_complete = true;
    }
  }
}
