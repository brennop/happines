#include "bus.h"
#include "ppu.h"

#include <stdio.h>

void bus_init(Bus *bus, Mapper *mapper, PPU *ppu, APU *apu, uint8_t *controller) {
  // reset ram

  bus->mapper = mapper;
  bus->ppu = ppu;
  bus->controller = controller;
  bus->apu = apu;

  bus->dma_page = 0x00;
  bus->dma_data = 0x00;
  bus->dma_addr = 0x00;
  bus->dma_dummy = true;
  bus->dma_transfer = false;
}

inline uint8_t bus_read(Bus *bus, uint16_t addr, bool read_only) {
  if (bus->mapper->prg_read(bus->mapper, addr)) {
    return *bus->mapper->prg_read(bus->mapper, addr);
  } else if (addr >= 0x0000 && addr <= 0x1FFF) {
    return bus->ram[addr & 0x7FFF];
  } else if (addr >= 0x2000 && addr <= 0x3FFF) {
    // ppu range
    return ppu_control_read(bus->ppu, addr & 0x0007, read_only);
  } else if (addr >= 0x4000 && addr <= 0x4013) {
    // apu range
  } else if (addr == 0x4014) {
    // oam dma
  } else if (addr == 0x4015) {
    return apu_read(bus->apu, addr);
  } else if (addr == 0x4016) {
    // controller 1
    uint8_t data = (bus->controller_state[0] & 0x80) > 0;
    bus->controller_state[0] <<= 1;
    return data;
  } else if (addr == 0x4017) {
    // controller 2
    uint8_t data = (bus->controller_state[1] & 0x80) > 0;
    bus->controller_state[1] <<= 1;
    return data;
  } else if (addr >= 0x4018 && addr <= 0x401F) {
    // apu range
  } else if (addr >= 0x4020 && addr <= 0xFFFF) {
    // mapper range
  }

  return 0;
}

void bus_write(Bus *bus, uint16_t addr, uint8_t data) {
  if (bus->mapper->prg_write(bus->mapper, addr, data)) {
    return;
  } else if (addr >= 0x0000 && addr <= 0x1FFF) {
    bus->ram[addr & 0x7FFF] = data;
  } else if (addr >= 0x2000 && addr <= 0x3FFF) {
    // ppu range
    ppu_control_write(bus->ppu, addr & 0x0007, data);
  } else if (addr >= 0x4000 && addr <= 0x4013) {
    // apu range
    apu_write(bus->apu, addr, data);
  } else if (addr == 0x4014) {
    bus->dma_page = data;
    bus->dma_addr = 0x00;
    bus->dma_transfer = true;
  } else if (addr == 0x4015 || addr == 0x4017) {
    // apu range
    apu_write(bus->apu, addr, data);
  } else if (addr == 0x4016) {
    // controller 1
    bus->controller_state[0] = bus->controller[0];
  } else if (addr == 0x4017) {
    // controller 2
    bus->controller_state[1] = bus->controller[1];
  } else if (addr >= 0x4018 && addr <= 0x401F) {
    // apu range
  } else if (addr >= 0x4020 && addr <= 0xFFFF) {
    // mapper range
  }
}

uint16_t bus_read_wide(Bus *bus, uint16_t addr, bool read_only) {
  uint16_t lo = bus_read(bus, addr, read_only);
  uint16_t hi = bus_read(bus, addr + 1, read_only);

  return (hi << 8) | lo;
}

void bus_dma_transfer(Bus *bus, int cycles) {
  // add dummy cycle
  if (bus->dma_dummy) {
    if (cycles % 2 == 1) {
      bus->dma_dummy = false;
    }
  } else {
    if (cycles % 2 == 0) {
      bus->dma_data = bus_read(bus, (bus->dma_page << 8) | bus->dma_addr, false);
    } else {
      ((uint8_t *)bus->ppu->oam)[bus->ppu->oam_addr] = bus->dma_data;
      bus->dma_addr++;
      bus->ppu->oam_addr++;

      if (bus->dma_addr == 0x00) {
        bus->dma_transfer = false;
        bus->dma_dummy = true;
      }
    }
  }
}
