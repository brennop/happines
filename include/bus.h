#ifndef __BUS_H__
#define __BUS_H__

#include "common.h"
#include "mapper.h"

typedef struct PPU PPU;

typedef struct {
  // 2KB of CPU RAM
  uint8_t ram[2048];

  Mapper *mapper;
  PPU *ppu;
} Bus;

void bus_init(Bus *bus, Mapper *mapper, PPU *ppu);
void bus_write(Bus *bus, uint16_t addr, uint8_t data);
uint8_t bus_read(Bus *bus, uint16_t addr, bool readonly);
uint16_t bus_read_wide(Bus *bus, uint16_t addr, bool readonly);

#endif // __BUS_H__
