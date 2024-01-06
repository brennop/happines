#include "ppu.h"

const uint32_t palette[] = {
    0x555555ff, 0x001773ff, 0x000786ff, 0x2e0578ff, 0x59024dff, 0x720011ff,
    0x6e0000ff, 0x4c0800ff, 0x171b00ff, 0x002a00ff, 0x003100ff, 0x002e08ff,
    0x002645ff, 0x000000ff, 0x000000ff, 0x000000ff, 0xa5a5a5ff, 0x0057c6ff,
    0x223fe5ff, 0x6e28d9ff, 0xae1aa6ff, 0xd21759ff, 0xd12107ff, 0xa73700ff,
    0x635100ff, 0x186700ff, 0x007200ff, 0x007331ff, 0x006a84ff, 0x000000ff,
    0x000000ff, 0x000000ff, 0xfeffffff, 0x2fa8ffff, 0x5d81ffff, 0x9c70ffff,
    0xf772ffff, 0xff77bdff, 0xff7e75ff, 0xff8a2bff, 0xcda000ff, 0x81b802ff,
    0x3dc830ff, 0x12cd7bff, 0x0dc5d0ff, 0x3c3c3cff, 0x000000ff, 0x000000ff,
    0xfeffffff, 0xa4deffff, 0xb1c8ffff, 0xccbeffff, 0xf4c2ffff, 0xffc5eaff,
    0xffc7c9ff, 0xffcdaaff, 0xefd696ff, 0xd0e095ff, 0xb3e7a5ff, 0x9feac3ff,
    0x9ae8e6ff, 0xafafafff, 0x000000ff, 0x000000ff};

void ppu_init(PPU *ppu, Bus *bus) {
  ppu->bus = bus;
  bus->ppu = ppu;
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

uint8_t ppu_control_read(PPU *ppu, uint16_t addr, bool readonly) {
  return 0;
}

void ppu_control_write(PPU *ppu, uint16_t addr, uint8_t data) {
}


uint32_t *ppu_get_pattern_table(PPU *ppu, int i) {
  for (int y = 0; y < 16; y++) {
    for (int x = 0; x < 16; x++) {
      /* int offset = (y * 16 + x) * 16; */

      for (int row = 0; row < 8; row++) {
      }
    }
  }

  return ppu->pattern_table[i];
}

