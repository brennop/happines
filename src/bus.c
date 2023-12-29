#include "bus.h"

void bus_init(Bus *bus) {
  // reset ram

}

void bus_write(Bus *bus, uint16_t addr, uint8_t data) {
  // ram range
  if (addr >= 0x0000 && addr <= 0xFFFF) {
    bus->ram[addr] = data;
  }
}

uint8_t bus_read(Bus *bus, uint16_t addr, bool read_only) {
  // ram range
  if (addr >= 0x0000 && addr <= 0xFFFF) {
    return bus->ram[addr];
  }

  return 0x00;
}

uint16_t bus_read_wide(Bus *bus, uint16_t addr, bool read_only) {
  uint16_t lo = bus_read(bus, addr, read_only);
  uint16_t hi = bus_read(bus, addr + 1, read_only);

  return (hi << 8) | lo;
}
