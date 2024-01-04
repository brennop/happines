#include "bus.h"

#include <stdio.h>

void bus_init(Bus *bus) {
  // reset ram
}

uint8_t *bus_get_ptr(Bus *bus, uint16_t addr, bool readonly) {
  if (bus->mapper.mapper_read(&bus->mapper, addr)) {
    return bus->mapper.mapper_read(&bus->mapper, addr);
  } else if (addr >= 0x0000 && addr <= 0x1FFF) {
    return bus->ram + (addr & 0x07FF);
  } else if (addr >= 0x2000 && addr <= 0x3FFF) {
    // ppu range
    return bus->ppu_ram + (addr & 0x0007);
  }

  return NULL;
}

inline uint8_t bus_read(Bus *bus, uint16_t addr, bool read_only) {
  return *bus_get_ptr(bus, addr, read_only);
}

void bus_write(Bus *bus, uint16_t addr, uint8_t data) {
  if (bus->mapper.mapper_write(&bus->mapper, addr, data)) {
    return;
  } else if (addr >= 0x0000 && addr <= 0x1FFF) {
    bus->ram[addr & 0x7FFF] = data;
  } else if (addr >= 0x2000 && addr <= 0x3FFF) {
    // ppu range
    bus->ppu_ram[addr & 0x0007] = data;
  }
}

// TODO: read with uint16_t
uint16_t bus_read_wide(Bus *bus, uint16_t addr, bool read_only) {
  uint16_t lo = bus_read(bus, addr, read_only);
  uint16_t hi = bus_read(bus, addr + 1, read_only);

  return (hi << 8) | lo;
}
