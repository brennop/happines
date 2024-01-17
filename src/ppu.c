#include "ppu.h"
#include <stdio.h>
#include <string.h>

const uint8_t mirror_lookup[][4] = {
    {0, 0, 1, 1}, // horizontal
    {0, 1, 0, 1}, // vertical
    {0, 0, 0, 0}, // single 0
    {1, 1, 1, 1}, // single 1
    {0, 1, 2, 3}, // four screen
};

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

uint8_t flip_byte(uint8_t byte) {
  byte = (byte & 0xF0) >> 4 | (byte & 0x0F) << 4;
  byte = (byte & 0xCC) >> 2 | (byte & 0x33) << 2;
  byte = (byte & 0xAA) >> 1 | (byte & 0x55) << 1;
  return byte;
}

uint8_t ppu_read(PPU *ppu, uint16_t addr, bool readonly) {
  uint8_t *mapper_data = ppu->mapper->chr_read(ppu->mapper, addr);

  if (mapper_data) {
    return *mapper_data;
  } else if (addr >= 0x2000 && addr <= 0x3EFF) { // nametable
    uint8_t mirror_mode = ppu->mapper->mirror(ppu->mapper);
    int mirror_addr = (addr - 0x2000) % 0x1000;
    uint8_t table = mirror_addr / 0x0400;
    int offset = mirror_addr % 0x0400;
    uint16_t index =
        mirror_lookup[mirror_mode][table] * 0x0400 + offset + 0x2000;
    return ppu->raw_nametable[index % 2048];
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
  } else if (addr >= 0x2000 && addr <= 0x3EFF) { // nametable
    uint8_t mirror_mode = ppu->mapper->mirror(ppu->mapper);
    int mirror_addr = (addr - 0x2000) % 0x1000;
    uint8_t table = mirror_addr / 0x0400;
    int offset = mirror_addr % 0x0400;
    uint16_t index =
        mirror_lookup[mirror_mode][table] * 0x0400 + offset + 0x2000;
    ppu->raw_nametable[index % 2048] = data;
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

static inline uint32_t ppu_get_color(PPU *ppu, uint8_t palette_id,
                                     uint8_t pixel) {
  return palette[ppu_read(ppu, 0x3F00 + (palette_id << 2) + pixel, false)];
}

uint8_t ppu_control_read(PPU *ppu, uint16_t addr, bool readonly) {
  uint8_t data = 0x00;
  switch (addr) {
  case 2:
    data = (ppu->status.reg & 0xE0) | (ppu->ppu_data_buffer & 0x1F);
    ppu->status.vertical_blank = 0;
    ppu->address_latch = 0;
    break;
  case 4: // OAM Data
    data = ((uint8_t *)ppu->oam)[ppu->oam_addr];
    break;
  case 7: // PPU Data
    data = ppu->ppu_data_buffer;
    ppu->ppu_data_buffer = ppu_read(ppu, ppu->vram_addr.reg, readonly);

    // palette read is not delayed
    if (ppu->vram_addr.reg >= 0x3F00) {
      data = ppu->ppu_data_buffer;
    }

    ppu->vram_addr.reg += ppu->control.increment_mode ? 32 : 1;

    break;
  }

  return data;
}

void ppu_control_write(PPU *ppu, uint16_t addr, uint8_t data) {
  switch (addr) {
  case 0:
    ppu->control.reg = data;
    ppu->tram_addr.nametable_x = ppu->control.nametable_x;
    ppu->tram_addr.nametable_y = ppu->control.nametable_y;
    break;
  case 1:
    ppu->mask.reg = data;
    break;
  case 3: // OAM Address
    ppu->oam_addr = data;
    break;
  case 4: // OAM Data
    ((uint8_t *)ppu->oam)[ppu->oam_addr] = data;
    break;
  case 5: // PPU Scroll
    if (ppu->address_latch == 0) {
      ppu->fine_x = data & 0x07;
      ppu->tram_addr.coarse_x = data >> 3;
      ppu->address_latch = 1;
    } else {
      ppu->tram_addr.fine_y = data & 0x07;
      ppu->tram_addr.coarse_y = data >> 3;
      ppu->address_latch = 0;
    }
    break;
  case 6: // PPU Address
    if (ppu->address_latch == 0) {
      ppu->tram_addr.reg =
          (ppu->tram_addr.reg & 0x00FF) | (uint16_t)((data & 0x3F) << 8);
      ppu->address_latch = 1;
    } else {
      ppu->tram_addr.reg = (ppu->tram_addr.reg & 0xFF00) | data;
      ppu->vram_addr = ppu->tram_addr;
      ppu->address_latch = 0;
    }
    break;
  case 7: // PPU Data
    ppu_write(ppu, ppu->vram_addr.reg, data);
    ppu->vram_addr.reg += ppu->control.increment_mode ? 32 : 1;

    break;
  }
}

void ppu_init(PPU *ppu, Mapper *mapper, uint8_t mirroring) {
  ppu->mapper = mapper;
  ppu->cycle = 0;
  ppu->scanline = 0;
}

void ppu_increment_scroll_x(PPU *ppu) {
  if (ppu->mask.render_background || ppu->mask.render_sprites) {
    if (ppu->vram_addr.coarse_x == 31) {
      ppu->vram_addr.coarse_x = 0;
      ppu->vram_addr.nametable_x = ~ppu->vram_addr.nametable_x;
    } else {
      ppu->vram_addr.coarse_x++;
    }
  }
}

void ppu_increment_scroll_y(PPU *ppu) {
  if (ppu->mask.render_background || ppu->mask.render_sprites) {
    if (ppu->vram_addr.fine_y < 7) {
      ppu->vram_addr.fine_y++;
    } else {
      ppu->vram_addr.fine_y = 0;

      if (ppu->vram_addr.coarse_y == 29) {
        ppu->vram_addr.coarse_y = 0;
        ppu->vram_addr.nametable_y = ~ppu->vram_addr.nametable_y;
      } else if (ppu->vram_addr.coarse_y == 31) {
        ppu->vram_addr.coarse_y = 0;
      } else {
        ppu->vram_addr.coarse_y++;
      }
    }
  }
}

void ppu_transfer_address_x(PPU *ppu) {
  if (ppu->mask.render_background || ppu->mask.render_sprites) {
    ppu->vram_addr.nametable_x = ppu->tram_addr.nametable_x;
    ppu->vram_addr.coarse_x = ppu->tram_addr.coarse_x;
  }
}

void ppu_transfer_address_y(PPU *ppu) {
  if (ppu->mask.render_background || ppu->mask.render_sprites) {
    ppu->vram_addr.fine_y = ppu->tram_addr.fine_y;
    ppu->vram_addr.nametable_y = ppu->tram_addr.nametable_y;
    ppu->vram_addr.coarse_y = ppu->tram_addr.coarse_y;
  }
}

void load_background_shifters(PPU *ppu) {
  ppu->bg_shifter_pattern_lo =
      (ppu->bg_shifter_pattern_lo & 0xFF00) | ppu->bg_next_tile_lsb;
  ppu->bg_shifter_pattern_hi =
      (ppu->bg_shifter_pattern_hi & 0xFF00) | ppu->bg_next_tile_msb;
  // expand the attribute to full byte
  ppu->bg_shifter_attrib_lo = (ppu->bg_shifter_attrib_lo & 0xFF00) |
                              ((ppu->bg_next_tile_attrib & 0b01) ? 0xFF : 0x00);
  ppu->bg_shifter_attrib_hi = (ppu->bg_shifter_attrib_hi & 0xFF00) |
                              ((ppu->bg_next_tile_attrib & 0b10) ? 0xFF : 0x00);
}

void update_shifters(PPU *ppu) {
  if (ppu->mask.render_background) {
    ppu->bg_shifter_pattern_lo <<= 1;
    ppu->bg_shifter_pattern_hi <<= 1;
    ppu->bg_shifter_attrib_lo <<= 1;
    ppu->bg_shifter_attrib_hi <<= 1;
  }

  if (ppu->mask.render_sprites && ppu->cycle >= 1 && ppu->cycle < 258) {
    for (uint8_t i = 0; i < ppu->sprite_count; i++) {
      if (ppu->sprite_scanline[i].x > 0) {
        ppu->sprite_scanline[i].x--;
      } else {
        ppu->sprite_shifter_pattern_lo[i] <<= 1;
        ppu->sprite_shifter_pattern_hi[i] <<= 1;
      }
    }
  }
}

void ppu_step(PPU *ppu) {
  if (ppu->scanline >= -1 && ppu->scanline < 240) {
    if (ppu->scanline == 0 && ppu->cycle == 0) {
      ppu->cycle = 1;
    }

    if (ppu->scanline == -1 && ppu->cycle == 1) {
      ppu->status.vertical_blank = 0;
      ppu->status.sprite_overflow = 0;
      ppu->status.sprite_zero_hit = 0;

      for (int i = 0; i < 8; i++) {
        ppu->sprite_shifter_pattern_lo[i] = 0;
        ppu->sprite_shifter_pattern_hi[i] = 0;
      }
    }

    // visible scanlines
    if ((ppu->cycle >= 2 && ppu->cycle < 258) ||
        (ppu->cycle >= 321 && ppu->cycle < 338)) {
      update_shifters(ppu);

      switch ((ppu->cycle - 1) & 0x07) {
      case 0:
        load_background_shifters(ppu);
        ppu->bg_next_tile_id =
            ppu_read(ppu, 0x2000 | (ppu->vram_addr.reg & 0x0FFF), false);
        break;
      case 2:
        ppu->bg_next_tile_attrib =
            ppu_read(ppu,
                     0x23C0 | (ppu->vram_addr.nametable_y << 11) |
                         (ppu->vram_addr.nametable_x << 10) |
                         ((ppu->vram_addr.coarse_y >> 2) << 3) |
                         (ppu->vram_addr.coarse_x >> 2),
                     false);
        if (ppu->vram_addr.coarse_y & 0x02) {
          ppu->bg_next_tile_attrib >>= 4;
        }

        if (ppu->vram_addr.coarse_x & 0x02) {
          ppu->bg_next_tile_attrib >>= 2;
        }

        ppu->bg_next_tile_attrib &= 0x03;

      case 4:
        ppu->bg_next_tile_lsb =
            ppu_read(ppu,
                     (ppu->control.pattern_background << 12) +
                         ((uint16_t)ppu->bg_next_tile_id << 4) +
                         (ppu->vram_addr.fine_y) + 0,
                     false);
        break;
      case 6:
        ppu->bg_next_tile_msb =
            ppu_read(ppu,
                     (ppu->control.pattern_background << 12) +
                         ((uint16_t)ppu->bg_next_tile_id << 4) +
                         (ppu->vram_addr.fine_y) + 8,
                     false);

        break;
      case 7:
        ppu_increment_scroll_x(ppu);
        break;
      }
    }

    if (ppu->cycle == 256) {
      ppu_increment_scroll_y(ppu);
    }

    if (ppu->cycle == 257) {
      ppu_transfer_address_x(ppu);
    }

    if (ppu->scanline == -1 && ppu->cycle >= 280 && ppu->cycle < 305) {
      ppu_transfer_address_y(ppu);
    }

    if (ppu->cycle == 338 || ppu->cycle == 340) {
      ppu->bg_next_tile_id =
          ppu_read(ppu, 0x2000 | (ppu->vram_addr.reg & 0x0FFF), false);
    }

    // foreground
    if (ppu->cycle == 257 && ppu->scanline >= 0) {
      memset(ppu->sprite_scanline, 0xFF, sizeof(ppu->sprite_scanline));
      ppu->sprite_count = 0;

      uint8_t entry = 0;
      // TODO: use for
      while (entry < 64 && ppu->sprite_count < 9) {
        int diff = ppu->scanline - ppu->oam[entry].y;

        if (diff >= 0 && diff < (ppu->control.sprite_size ? 16 : 8)) {
          if (ppu->sprite_count < 8) {
            memcpy(&ppu->sprite_scanline[ppu->sprite_count], &ppu->oam[entry],
                   sizeof(Sprite));
            ppu->sprite_count++;
          }
        }

        entry++;
      }
    }
    ppu->status.sprite_overflow = ppu->sprite_count > 8;
  }

  if (ppu->cycle == 340) {
    for (uint8_t i = 0; i < ppu->sprite_count; i++) {
      uint8_t sprite_pattern_bits_lo, sprite_pattern_bits_hi;
      uint16_t sprite_pattern_addr_lo, sprite_pattern_addr_hi;

      Sprite *sprite = &ppu->sprite_scanline[i];

      if (ppu->control.sprite_size) {    // 16
        if (sprite->attributes & 0x80) { // flipped
          if (ppu->scanline - sprite->y < 8) {
            sprite_pattern_addr_lo = ((sprite->tile_id & 0x01) << 12) |
                                     ((sprite->tile_id & 0xFE) << 4) |
                                     ((7 - (ppu->scanline - sprite->y)) & 0x07);
          } else {
            sprite_pattern_addr_lo = ((sprite->tile_id & 0x01) << 12) |
                                     (((sprite->tile_id & 0xFE) + 1) << 4) |
                                     ((7 - (ppu->scanline - sprite->y)) & 0x07);
          }
        } else {
          if (ppu->scanline - sprite->y < 8) {
            sprite_pattern_addr_lo = ((sprite->tile_id & 0x01) << 12) |
                                     ((sprite->tile_id & 0xFE) << 4) |
                                     ((ppu->scanline - sprite->y) & 0x07);
          } else {
            sprite_pattern_addr_lo = ((sprite->tile_id & 0x01) << 12) |
                                     (((sprite->tile_id & 0xFE) + 1) << 4) |
                                     ((ppu->scanline - sprite->y) & 0x07);
          }
        }
      } else {                           // 8
        if (sprite->attributes & 0x80) { // flipped
          sprite_pattern_addr_lo = (ppu->control.pattern_sprite << 12) |
                                   (sprite->tile_id << 4) |
                                   (7 - (ppu->scanline - sprite->y));
        } else {
          sprite_pattern_addr_lo = (ppu->control.pattern_sprite << 12) |
                                   (sprite->tile_id << 4) |
                                   (ppu->scanline - sprite->y);
        }
      }

      sprite_pattern_bits_lo = ppu_read(ppu, sprite_pattern_addr_lo, false);
      sprite_pattern_addr_hi = sprite_pattern_addr_lo + 8;
      sprite_pattern_bits_hi = ppu_read(ppu, sprite_pattern_addr_hi, false);

      if (sprite->attributes & 0x40) {
        sprite_pattern_bits_lo = flip_byte(sprite_pattern_bits_lo);
        sprite_pattern_bits_hi = flip_byte(sprite_pattern_bits_hi);
      }

      ppu->sprite_shifter_pattern_lo[i] = sprite_pattern_bits_lo;
      ppu->sprite_shifter_pattern_hi[i] = sprite_pattern_bits_hi;
    }
  }

  if (ppu->scanline == 240) {
    // do nothing
  }

  if (ppu->scanline == 241 && ppu->cycle == 1) {
    ppu->status.vertical_blank = 1;

    if (ppu->control.enable_nmi) {
      ppu->nmi = true;
    }
  }

  uint8_t bg_pixel = 0x00;
  uint8_t bg_palette = 0x00;

  if (ppu->mask.render_background) {
    uint16_t bit_mux = 0x8000 >> ppu->fine_x;

    uint8_t p0_pixel = (ppu->bg_shifter_pattern_lo & bit_mux) > 0;
    uint8_t p1_pixel = (ppu->bg_shifter_pattern_hi & bit_mux) > 0;
    bg_pixel = (p1_pixel << 1) | p0_pixel;

    uint8_t bg_pal0 = (ppu->bg_shifter_attrib_lo & bit_mux) > 0;
    uint8_t bg_pal1 = (ppu->bg_shifter_attrib_hi & bit_mux) > 0;
    bg_palette = (bg_pal1 << 1) | bg_pal0;
  }

  uint8_t fg_pixel = 0x00;
  uint8_t fg_palette = 0x00;
  uint8_t fg_priority = 0x00;

  if (ppu->mask.render_sprites) {
    for (uint8_t i = 0; i < ppu->sprite_count; i++) {
      Sprite *sprite = &ppu->sprite_scanline[i];
      if (sprite->x == 0) {
        uint8_t fg_pixel_lo = (ppu->sprite_shifter_pattern_lo[i] & 0x80) > 0;
        uint8_t fg_pixel_hi = (ppu->sprite_shifter_pattern_hi[i] & 0x80) > 0;
        fg_pixel = (fg_pixel_hi << 1) | fg_pixel_lo;

        fg_palette = (sprite->attributes & 0x03) + 0x04;
        fg_priority = (sprite->attributes & 0x20) == 0;

        if (fg_pixel != 0) {
          if (i == 0) {
            ppu->status.sprite_zero_hit = 1;
          }

          break;
        }
      }
    }
  }

  uint8_t pixel = 0x00;
  uint8_t palette = 0x00;

  if (bg_pixel == 0 && fg_pixel == 0) {
    pixel = 0;
    palette = 0;
  } else if (bg_pixel == 0 && fg_pixel > 0) {
    pixel = fg_pixel;
    palette = fg_palette;
  } else if (bg_pixel > 0 && fg_pixel == 0) {
    pixel = bg_pixel;
    palette = bg_palette;
  } else {
    if (fg_priority) {
      pixel = fg_pixel;
      palette = fg_palette;
    } else {
      pixel = bg_pixel;
      palette = bg_palette;
    }
  }

  // boundary check
  if (ppu->scanline >= 0 && ppu->scanline < 240 && ppu->cycle < 256) {
    int pixel_index = ppu->scanline * 256 + ppu->cycle - 1;
    ppu->framebuffer[pixel_index] = ppu_get_color(ppu, palette, pixel);
  }

  ppu->cycle++;

  if (ppu->mask.render_background || ppu->mask.render_sprites) {
    if (ppu->cycle == 260 && ppu->scanline < 240) {
      ppu->mapper->scanline(ppu->mapper);
    }
  }

  if (ppu->cycle >= 341) {
    ppu->cycle = 0;
    ppu->scanline++;

    if (ppu->scanline == 261) {
      ppu->scanline = -1;
      ppu->frame_complete = true;
    }
  }
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
          uint8_t pixel = ((tile_lsb & 0x01) << 1) | (tile_msb & 0x01);
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
