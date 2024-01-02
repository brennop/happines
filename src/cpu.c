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

#define CASE5(x)                                                               \
  x:                                                                           \
  case x + 0x04:                                                                  \
  case x + 0x08:                                                                  \
  case x + 0x10:                                                                 \
  case x + 0x18

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

static inline void push(CPU *cpu, uint8_t data) {
  bus_write(cpu->bus, 0x0100 + cpu->sp--, data);
}

static inline uint8_t pop(CPU *cpu) {
  return bus_read(cpu->bus, 0x0100 + ++cpu->sp, false);
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
  cpu_set_flag(cpu, FLAG_OVERFLOW,
               (~(cpu->a ^ data) & (cpu->a ^ result)) & 0x80);
  cpu->a = result & 0x00FF;
  return 1;
}

static inline void asl(CPU *cpu, uint8_t *ptr) {
  uint16_t result = *ptr << 1;
  *ptr = result & 0x00FF;
  cpu_set_flag(cpu, FLAG_CARRY, result & 0xFF00);
  cpu_set_flag(cpu, FLAG_ZERO, *ptr == 0x00);
  cpu_set_flag(cpu, FLAG_NEGATIVE, *ptr & 0x80);
}

static inline void rti(CPU *cpu) {
  cpu->status = pop(cpu);
  cpu->status &= ~FLAG_BREAK;
  cpu->status &= ~FLAG_UNUSED;

  cpu->pc = pop(cpu);
  cpu->pc |= (uint16_t)pop(cpu) << 8;
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
  uint8_t* ptr;

  // Addressing modes
  switch (instruction.addressing_mode) {
  case ADDR_MODE_IMP:
    data = cpu->a;
    ptr = &cpu->a;
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
    ptr = bus_get_ptr(cpu->bus, addr, false);
    data = *ptr;
  }

  switch (opcode) {
  case CASE8_4(0x61): // ADC
    operation_cycles = adc(cpu, data);
    break;
  case CASE8_4(0x21): // AND
    cpu->a &= data;
    cpu_set_flag(cpu, FLAG_ZERO, cpu->a == 0x00);
    cpu_set_flag(cpu, FLAG_NEGATIVE, cpu->a & 0x80);
    operation_cycles = 1;
    break;
  case CASE5(0x06): // ASL
    asl(cpu, ptr);
    break;
  // FIXME: f1 takes too long
  case CASE8_4(0xE1):
    operation_cycles = adc(cpu, ~data);
    break;
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
  case 0x40: // RTI
    rti(cpu);
    break;
  case 0xB8: // CLV
    cpu_set_flag(cpu, FLAG_OVERFLOW, false);
    break;
  case 0x48: // PHA
    bus_write(cpu->bus, 0x0100 + cpu->sp--, cpu->a);
    break;
  case 0x68: // PLA
    cpu->a = bus_read(cpu->bus, 0x0100 + ++cpu->sp, false);
    cpu_set_flag(cpu, FLAG_ZERO, cpu->a == 0x00);
    cpu_set_flag(cpu, FLAG_NEGATIVE, cpu->a & 0x80);
    break;
  default:
    printf("Unimplemented opcode: %02X\n", opcode);
    exit(1);
    break;
  }

  cpu->cycles = cycles + (addressing_cycles & operation_cycles);
}

void cpu_reset(CPU *cpu) {
  cpu->a = 0;
  cpu->x = 0;
  cpu->y = 0;
  cpu->sp = 0xFD;
  cpu->status = 0x00 | FLAG_UNUSED; // Unused flag is always 1

  cpu->pc = bus_read_wide(cpu->bus, 0xFFFC, false);

  cpu->cycles = 8;
}

void cpu_irq(CPU *cpu) {
  if (!(cpu->status & FLAG_INTERRUPT_DISABLE)) {
    // save pc
    push(cpu, (cpu->pc >> 8) & 0x00FF);
    push(cpu, cpu->pc & 0x00FF);

    cpu_set_flag(cpu, FLAG_BREAK, false);
    cpu_set_flag(cpu, FLAG_INTERRUPT_DISABLE, true);
    cpu_set_flag(cpu, FLAG_UNUSED, true);

    // save status
    push(cpu, cpu->status);

    cpu->pc = bus_read_wide(cpu->bus, 0xFFFE, false);

    cpu->cycles = 7;
  }
}

void cpu_nmi(CPU *cpu) {
  // save pc
  push(cpu, (cpu->pc >> 8) & 0x00FF);
  push(cpu, cpu->pc & 0x00FF);

  cpu_set_flag(cpu, FLAG_BREAK, false);
  cpu_set_flag(cpu, FLAG_INTERRUPT_DISABLE, true);
  cpu_set_flag(cpu, FLAG_UNUSED, true);

  // save status
  push(cpu, cpu->status);

  cpu->pc = bus_read_wide(cpu->bus, 0xFFFA, false);

  cpu->cycles = 8;
}
