#ifndef __CPU_H__
#define __CPU_H__

#include "common.h"
#include "bus.h"

typedef enum {
  CARRY_FLAG = 0x01,
  ZERO_FLAG = 0x02,
  INTERRUPT_DISABLE_FLAG = 0x04,
  DECIMAL_MODE_FLAG = 0x08, // unused in NES
  BREAK_FLAG = 0x10,
  UNUSED_FLAG = 0x20,
  OVERFLOW_FLAG = 0x40,
  NEGATIVE_FLAG = 0x80
} StatusFlag;

typedef struct {
  uint8_t a; // accumulator
  uint8_t x; // x register
  uint8_t y; // y register
  uint8_t sp; // stack pointer
  uint16_t pc; // program counter
  uint8_t status; // status register

  uint8_t fetched; // fetched data
  uint16_t addr_abs; // absolute address
  uint16_t addr_rel; // relative address

  int cycles;

  Bus *bus;
} CPU;

void cpu_init(CPU *cpu, Bus *bus);
void cpu_step(CPU *cpu);

void cpu_reset(CPU *cpu);
void cpu_irq(CPU *cpu);
void cpu_nmi(CPU *cpu);

#endif // __CPU_H__
