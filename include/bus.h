#ifndef __BUS_H__
#define __BUS_H__

#include "common.h"
#include "mapper.h"

typedef struct PPU PPU;

typedef struct {
  // 2KB of CPU RAM
  uint8_t ram[2048];

  // input
  uint8_t *controller;
  uint8_t controller_state[2];

  // dma
  uint8_t dma_page;
  uint8_t dma_addr;
  uint8_t dma_data;
  /** is a dma transfer in progress? */ 
  uint8_t dma_transfer;
  /** dummy read */
  uint8_t dma_dummy;

  Mapper *mapper;
  PPU *ppu;
} Bus;

void bus_init(Bus *bus, Mapper *mapper, PPU *ppu, uint8_t *controller);
void bus_write(Bus *bus, uint16_t addr, uint8_t data);
uint8_t bus_read(Bus *bus, uint16_t addr, bool readonly);
uint16_t bus_read_wide(Bus *bus, uint16_t addr, bool readonly);
void bus_dma_transfer(Bus *bus, int cycles);

#endif // __BUS_H__
