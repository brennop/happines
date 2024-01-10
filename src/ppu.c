#include "ppu.h"
#include <stdio.h>

const uint32_t palette[] = {
    0xff555555, 0xff731700, 0xff860700, 0xff78052e, 0xff4d0259, 0xff110072,
    0xff00006e, 0xff00084c, 0xff001b17, 0xff002a00, 0xff003100, 0xff082e00,
    0xff452600, 0xff000000, 0xff000000, 0xff000000, 0xffa5a5a5, 0xffc65700,
    0xffe53f22, 0xffd9286e, 0xffa61aae, 0xff5917d2, 0xff0721d1, 0xff0037a7,
    0xff005163, 0xff006718, 0xff007200, 0xff317300, 0xff846a00, 0xff000000,
    0xff000000, 0xff000000, 0xfffffffe, 0xffffa82f, 0xffff815d, 0xffff709c,
    0xffff72f7, 0xffbd77ff, 0xff757eff, 0xff2b8aff, 0xff00a0cd, 0xff02b881,
    0xff30c83d, 0xff7bcd12, 0xffd0c50d, 0xff3c3c3c, 0xff000000, 0xff000000,
    0xfffffffe, 0xffffdea4, 0xffffc8b1, 0xffffbecc, 0xffffc2f4, 0xffeac5ff,
    0xffc9c7ff, 0xffaacdff, 0xff96d6ef, 0xff95e0d0, 0xffa5e7b3, 0xffc3ea9f,
    0xffe6e89a, 0xffafafaf, 0xff000000, 0xff000000};

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

uint8_t ppu_control_read(PPU *ppu, uint16_t addr, bool readonly) {
  uint8_t data = 0x00;
  switch (addr) {
  case 2:
    data = (ppu->status.reg & 0xE0) | (ppu->ppu_data_buffer & 0x1F);
    ppu->status.vertical_blank = 0;
    ppu->address_latch = 0;
    break;
  case 7: // PPU Data
    data = ppu->ppu_data_buffer;
    ppu->ppu_data_buffer = ppu_read(ppu, ppu->ppu_addr, readonly);

    // palette read is not delayed
    if (ppu->ppu_addr >= 0x3F00) {
      data = ppu->ppu_data_buffer;
    }

    ppu->ppu_addr += 1;
    /* if (ppu->control.increment_mode == 0) { */
    /* } else { */
    /*   ppu->ppu_addr += 32; */
    /* } */

    break;
  }

  return data;
}

void ppu_control_write(PPU *ppu, uint16_t addr, uint8_t data) {
  switch (addr) {
  case 0:
    ppu->control.reg = data;
    break;
  case 1:
    ppu->mask.reg = data;
    break;
  case 6: // PPU Address
    if (ppu->address_latch == 0) {
      ppu->ppu_addr = (ppu->ppu_addr & 0x00FF) | (data << 8);
      ppu->address_latch = 1;
    } else {
      ppu->ppu_addr = (ppu->ppu_addr & 0xFF00) | data;
      ppu->address_latch = 0;
    }

    ppu->ppu_addr += 1;
    break;
  case 7: // PPU Data
    ppu_write(ppu, ppu->ppu_addr, data);
    ppu->ppu_addr += 1;

    break;
  }
}

void ppu_init(PPU *ppu, Mapper *mapper) { ppu->mapper = mapper; }

void ppu_step(PPU *ppu) {
  if (ppu->scanline == -1 && ppu->cycle == 1) {
    ppu->status.vertical_blank = 0;
    ppu->status.sprite_overflow = 0;
    ppu->status.sprite_zero_hit = 0;
  }

  if (ppu->scanline == 241 && ppu->cycle == 1) {
    ppu->status.vertical_blank = 1;

    if (ppu->control.enable_nmi) {
      ppu->nmi = true;
    }
  }

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


static inline uint32_t ppu_get_color(PPU *ppu, uint8_t palette_id,
                                     uint8_t pixel) {
  return palette[ppu_read(ppu, 0x3F00 + (palette_id << 2) + pixel, false)];
}

uint32_t *ppu_get_pattern_table(PPU *ppu, uint8_t i, uint8_t palette) {
  for (int y = 0; y < 16; y++) {
    for (int x = 0; x < 16; x++) {
      int offset = y * 256 + x * 16;

      for (int row = 0; row < 8; row++) {
        // read from the pattern table
        uint8_t tile_lsb = ppu_read(ppu, i * 0x1000 + offset + row, false);
        uint8_t tile_msb = ppu_read(ppu, i * 0x1000 + offset + row + 8, false);

        for (int col = 0; col < 8; col++) {
          uint8_t pixel = (tile_lsb & 0x01) + (tile_msb & 0x01);
          // FIXME: no mutation
          tile_lsb >>= 1;
          tile_msb >>= 1;

          int pixel_x = x * 8 + (7 - col);
          int pixel_y = y * 8 + row;
          int index = pixel_y * 128 + pixel_x;
          ppu->pattern_table[i][index] = ppu_get_color(ppu, palette, pixel);
        }
      }
    }
  }

  return ppu->pattern_table[i];
}
