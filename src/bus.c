#include "bus.h"
#include "ppu.h"

#include <stdio.h>

void bus_init(Bus *bus) {
  // reset ram
}

inline uint8_t bus_read(Bus *bus, uint16_t addr, bool read_only) {
  if (bus->mapper.prg_read(&bus->mapper, addr)) {
    return *bus->mapper.prg_read(&bus->mapper, addr);
  } else if (addr >= 0x0000 && addr <= 0x1FFF) {
    return bus->ram[addr & 0x7FFF];
  } else if (addr >= 0x2000 && addr <= 0x3FFF) {
    // ppu range
    return ppu_control_read(bus->ppu, addr & 0x0007, read_only);
  }

  return 0;
}

void bus_write(Bus *bus, uint16_t addr, uint8_t data) {
  if (bus->mapper.prg_write(&bus->mapper, addr, data)) {
    return;
  } else if (addr >= 0x0000 && addr <= 0x1FFF) {
    bus->ram[addr & 0x7FFF] = data;
  } else if (addr >= 0x2000 && addr <= 0x3FFF) {
    // ppu range
    ppu_control_write(bus->ppu, addr & 0x0007, data);
  }
}

uint16_t bus_read_wide(Bus *bus, uint16_t addr, bool read_only) {
  uint16_t lo = bus_read(bus, addr, read_only);
  uint16_t hi = bus_read(bus, addr + 1, read_only);

  return (hi << 8) | lo;
}
