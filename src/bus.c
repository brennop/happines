#include "bus.h"

#include <stdio.h>

void bus_init(Bus *bus) {
  // reset ram

}

void bus_write(Bus *bus, uint16_t addr, uint8_t data) {
  // ram range
  if (addr >= 0x0000 && addr <= 0xFFFF) {
    bus->ram[addr] = data;
  }
}

inline uint8_t bus_read(Bus *bus, uint16_t addr, bool read_only) {
  return *bus_get_ptr(bus, addr, read_only);
}

uint8_t* bus_get_ptr(Bus *bus, uint16_t addr, bool readonly) {
  // ram range
  if (addr >= 0x0000 && addr <= 0xFFFF) {
    return bus->ram + addr;
  }

  return NULL;
}

uint16_t bus_read_wide(Bus *bus, uint16_t addr, bool read_only) {
  uint16_t lo = bus_read(bus, addr, read_only);
  uint16_t hi = bus_read(bus, addr + 1, read_only);

  return (hi << 8) | lo;
}
