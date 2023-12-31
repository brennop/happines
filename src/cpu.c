#include "cpu.h"
#include "instructions.h"

#include <stdio.h>
#include <stdlib.h>

const uint8_t BRANCH_OFF[] = {7, 6, 0, 1};

#define CASE8_4(x)                                                             \
  x:                                                                           \
  case x + 4:                                                                  \
  case x + 8:                                                                  \
  case x + 12:                                                                 \
  case x + 16:                                                                 \
  case x + 20:                                                                 \
  case x + 24:                                                                 \
  case x + 28

#define VERT_8(x)                                                              \
  x:                                                                           \
  case x + 0x20:                                                               \
  case x + 0x40:                                                               \
  case x + 0x60:                                                               \
  case x + 0x80:                                                               \
  case x + 0xA0:                                                               \
  case x + 0xC0:                                                               \
  case x + 0xE0

#define VERT_4(x)                                                              \
  x:                                                                           \
  case x + 0x20:                                                               \
  case x + 0x40:                                                               \
  case x + 0x60

#define CHECK_PAGE_CROSSING(addr1, addr2)                                      \
  if ((addr1 & 0xFF00) != (addr2 & 0xFF00)) {                                  \
    cycles++;                                                                  \
  }

void cpu_bus_write(CPU *cpu, uint16_t addr, uint8_t data) {
  bus_write(cpu->bus, addr, data);
}

uint8_t cpu_bus_read(CPU *cpu, uint16_t addr, bool readonly) {
  return bus_read(cpu->bus, addr, readonly);
}

static inline void cpu_set_flag(CPU *cpu, uint8_t mask, bool value) {
  if (value) {
    cpu->status |= mask;
  } else {
    cpu->status &= ~mask;
  }
}

static inline uint8_t adc(CPU *cpu, uint8_t data) {
  uint16_t result = cpu->a + data + (cpu->status & FLAG_CARRY);
  cpu_set_flag(cpu, FLAG_CARRY, result > 0xFF);
  cpu_set_flag(cpu, FLAG_ZERO, (result & 0x00FF) == 0);
  cpu_set_flag(cpu, FLAG_NEGATIVE, result & 0x80);
  cpu_set_flag(cpu, FLAG_OVERFLOW, (~(cpu->a ^ data) & (cpu->a ^ result)) & 0x80);
  cpu->a = result & 0x00FF;
  return 1;
}

void cpu_init(CPU *cpu, Bus *bus) { cpu->bus = bus; }

void cpu_step(CPU *cpu) {
  uint8_t opcode = bus_read(cpu->bus, cpu->pc, false);
  Instruction instruction = instructions[opcode];

  cpu->pc++;
  uint8_t cycles = instruction.cycles;
  uint8_t addressing_cycles = 0;
  uint8_t operation_cycles = 0;

  uint16_t addr;
  uint8_t data;

  // Addressing modes
  switch (instruction.addressing_mode) {
  case ADDR_MODE_IMP:
    data = cpu->a;
    break;
  case ADDR_MODE_IMM:
    addr = cpu->pc++;
    break;
  case ADDR_MODE_ZP0:
    addr = bus_read(cpu->bus, cpu->pc++, false) & 0x00FF;
    break;
  case ADDR_MODE_ZPX:
    addr = (bus_read(cpu->bus, cpu->pc++, false) + cpu->x) & 0x00FF;
    break;
  case ADDR_MODE_ZPY:
    addr = (bus_read(cpu->bus, cpu->pc++, false) + cpu->y) & 0x00FF;
    break;
  case ADDR_MODE_ABS:
    addr = bus_read(cpu->bus, cpu->pc++, false);
    addr |= (uint16_t)(bus_read(cpu->bus, cpu->pc++, false) << 8);
    break;
  case ADDR_MODE_ABX: {
    addr = bus_read(cpu->bus, cpu->pc++, false);
    uint16_t addr_abs_hi = bus_read(cpu->bus, cpu->pc++, false);
    addr |= (addr_abs_hi << 8);
    addr += cpu->x;

    if ((addr & 0xFF00) != (addr_abs_hi << 8))
      addressing_cycles++;
    break;
  }
  case ADDR_MODE_ABY: {
    addr = bus_read(cpu->bus, cpu->pc++, false);
    uint16_t addr_abs_hi = bus_read(cpu->bus, cpu->pc++, false);
    addr |= (addr_abs_hi << 8);
    addr += cpu->y;

    if ((addr & 0xFF00) != (addr_abs_hi << 8))
      addressing_cycles++;
    break;
  }
  case ADDR_MODE_IND: {
    uint16_t ptr_lo = bus_read(cpu->bus, cpu->pc++, false);
    uint16_t ptr_hi = bus_read(cpu->bus, cpu->pc++, false);
    uint16_t ptr = (ptr_hi << 8) | ptr_lo;

    addr = bus_read_wide(cpu->bus, ptr, false);

    // TODO: check for page boundary crossing
    break;
  }
  case ADDR_MODE_IZX: {
    uint16_t t = bus_read(cpu->bus, cpu->pc++, false);
    uint16_t lo =
        bus_read(cpu->bus, (uint16_t)(t + (uint16_t)cpu->x) & 0x00FF, false);
    uint16_t hi = bus_read(
        cpu->bus, (uint16_t)(t + (uint16_t)cpu->x + 1) & 0x00FF, false);

    addr = (hi << 8) | lo;
    break;
  }
  case ADDR_MODE_IZY: {
    uint16_t t = bus_read(cpu->bus, cpu->pc++, false);
    uint16_t lo = bus_read(cpu->bus, t & 0x00FF, false);
    uint16_t hi = bus_read(cpu->bus, (t + 1) & 0x00FF, false);
    addr = (hi << 8) | lo;
    addr += cpu->y;

    if ((addr & 0xFF00) != (hi << 8))
      addressing_cycles++;
    break;
  }
  case ADDR_MODE_REL: {
    addr = bus_read(cpu->bus, cpu->pc++, false);
    if (addr & 0x80) {
      addr |= 0xFF00;
    }
    break;
  }
  }

  if (instruction.addressing_mode != ADDR_MODE_IMP) {
    data = bus_read(cpu->bus, addr, false);
  }

  switch (opcode) {
  case CASE8_4(0x21): // AND
    cpu->a &= data;
    cpu_set_flag(cpu, FLAG_ZERO, cpu->a == 0x00);
    cpu_set_flag(cpu, FLAG_NEGATIVE, cpu->a & 0x80);
    operation_cycles = 1;
    break;
  case CASE8_4(0x61): operation_cycles = adc(cpu, data); break;
  case VERT_8(0x10): // Branches
    if ((cpu->status >> BRANCH_OFF[opcode >> 6] & 0x01) ==
        ((opcode >> 5) & 0x01)) {
      cycles++;
      addr = cpu->pc + addr;
      // TODO: sum instead of branch
      CHECK_PAGE_CROSSING(addr, cpu->pc);
      cpu->pc = addr;
    }
    break;
  case VERT_4(0x18):
    /**          flag mask value
     * CLC 0x18   C   0x01  0
     * SEC 0x38   C   0x01  1
     * CLI 0x58   I   0x04  0
     * SEI 0x78   I   0x04  1
     * mask = 2 ** (opcode >> 6) * 2
     * value = bit 5 of opcode
     */
    cpu_set_flag(cpu, 1 << ((opcode >> 6) << 1), opcode & 0x20);
    break;
  case 0xB8: // CLV
    cpu_set_flag(cpu, FLAG_OVERFLOW, false);
    break;
  default:
    printf("Unimplemented opcode: %02X\n", opcode);
    exit(1);
    break;
  }

  cpu->cycles = cycles + (addressing_cycles & operation_cycles);
}

void cpu_reset(CPU *cpu);
void cpu_irq(CPU *cpu);
void cpu_nmi(CPU *cpu);
