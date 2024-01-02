#ifndef __CPU_H__
#define __CPU_H__

#include "bus.h"
#include "common.h"

typedef enum {
  FLAG_CARRY = 0x01,
  FLAG_ZERO = 0x02,
  FLAG_INTERRUPT_DISABLE = 0x04,
  FLAG_DECIMAL_MODE = 0x08, // unused in NES
  FLAG_BREAK = 0x10,
  FLAG_UNUSED = 0x20,
  FLAG_OVERFLOW = 0x40,
  FLAG_NEGATIVE = 0x80
} StatusFlag;

typedef struct {
  uint8_t a;      // accumulator
  uint8_t x;      // x register
  uint8_t y;      // y register
  uint8_t sp;     // stack pointer
  uint16_t pc;    // program counter
  uint8_t status; // status register

  int cycles;

  Bus *bus;
} CPU;

void cpu_init(CPU *cpu, Bus *bus);
uint8_t cpu_step(CPU *cpu);

void cpu_reset(CPU *cpu);
void cpu_irq(CPU *cpu);
void cpu_nmi(CPU *cpu);

// library functions
extern void cpu_bus_write(CPU *cpu, uint16_t addr, uint8_t data);
extern uint8_t cpu_bus_read(CPU *cpu, uint16_t addr, bool readonly);

#endif // __CPU_H__
