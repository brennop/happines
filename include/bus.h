#ifndef __BUS_H__
#define __BUS_H__

#include "common.h"
#include "mapper.h"

typedef struct {
  // 2KB of CPU RAM
  uint8_t ram[2048];
  // 2KB of PPU RAM
  uint8_t ppu_ram[2048];

  Mapper mapper;
} Bus;

void bus_init(Bus *bus);
void bus_write(Bus *bus, uint16_t addr, uint8_t data);
uint8_t bus_read(Bus *bus, uint16_t addr, bool readonly);
uint8_t *bus_get_ptr(Bus *bus, uint16_t addr, bool readonly);
uint16_t bus_read_wide(Bus *bus, uint16_t addr, bool readonly);

#endif // __BUS_H__
