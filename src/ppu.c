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

uint8_t ppu_read(PPU *ppu, uint16_t addr, bool readonly) {
  uint8_t *mapper_data = ppu->mapper->chr_read(ppu->mapper, addr);

  if (mapper_data) {
    return *mapper_data;
  } else if (addr >= 0x0000 && addr <= 0x1FFF) { // pattern table
    return ppu->raw_pattern_table[(addr & 0x1000) >> 12][addr & 0x0FFF];
  } else if (addr >= 0x2000 && addr <= 0x3EFF) { // nametable
  } else if (addr >= 0x3F00 && addr <= 0x3FFF) { // palette
    addr &= 0x001F;

    if (addr == 0x0010)
      addr = 0x0000;
    else if (addr == 0x0014)
      addr = 0x0004;
    else if (addr == 0x0018)
      addr = 0x0008;
    else if (addr == 0x001C)
      addr = 0x000C;

    return ppu->palette[addr];
  }

  return 0;
}

void ppu_write(PPU *ppu, uint16_t addr, uint8_t data) {
  if (ppu->mapper->chr_write(ppu->mapper, addr, data)) {
  } else if (addr >= 0x0000 && addr <= 0x1FFF) { // pattern table
    ppu->raw_pattern_table[(addr & 0x1000) >> 12][addr & 0x0FFF] = data;
  } else if (addr >= 0x2000 && addr <= 0x3EFF) { // nametable
  } else if (addr >= 0x3F00 && addr <= 0x3FFF) { // palette
    addr &= 0x001F;

    if (addr == 0x0010)
      addr = 0x0000;
    else if (addr == 0x0014)
      addr = 0x0004;
    else if (addr == 0x0018)
      addr = 0x0008;
    else if (addr == 0x001C)
      addr = 0x000C;

    ppu->palette[addr] = data;
  }
}

void ppu_init(PPU *ppu, Mapper *mapper) { ppu->mapper = mapper; }

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

uint8_t ppu_control_read(PPU *ppu, uint16_t addr, bool readonly) { return 0; }

void ppu_control_write(PPU *ppu, uint16_t addr, uint8_t data) {}

static inline uint32_t ppu_get_color(PPU *ppu, uint8_t palette_id,
                                     uint8_t pixel) {
  return palette[ppu_read(ppu, 0x3F00 + (palette_id << 2) + pixel, false)];
}

uint32_t *ppu_get_pattern_table(PPU *ppu, uint8_t i, uint8_t palette) {
  for (int y = 0; y < 16; y++) {
    for (int x = 0; x < 16; x++) {
      int offset = (y * 16 + x) * 16;

      for (int row = 0; row < 8; row++) {
        // read from the pattern table
        uint8_t tile_lsb = ppu_read(ppu, i * 0x1000 + offset + row, false);
        uint8_t tile_msb = ppu_read(ppu, i * 0x1000 + offset + row + 8, false);

        for (int col = 0; col < 8; col++) {
          uint8_t pixel = (tile_lsb & 0x01) + (tile_msb & 0x01);
          // FIXME: no mutation
          tile_lsb >>= 1;
          tile_msb >>= 1;

          uint8_t pixel_x = x * 8 + (7 - col);
          uint8_t pixel_y = y * 8 + row;
          uint8_t index = pixel_y * 128 + pixel_x;
          ppu->pattern_table[i][index] = ppu_get_color(ppu, palette, pixel);
        }
      }
    }
  }

  return ppu->pattern_table[i];
}
