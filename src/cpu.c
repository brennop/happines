#include "cpu.h"
#include "instructions.h"

#include <stdio.h>
#include <stdlib.h>

const uint8_t BRANCH_OFF[] = {7, 6, 0, 1};

void cpu_trace(Instruction instruction, uint16_t addr, uint8_t data) {
  printf("0x%04X: %s %02X \n", addr, instruction.mnemonic, data);
}

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

#define CASE4(x)                                                               \
  x:                                                                           \
  case x + 0x08:                                                               \
  case x + 0x10:                                                               \
  case x + 0x18

#define CASE5(x) CASE4(x) : case x + 0x04

#define CHECK_PAGE_CROSSING(addr1, addr2)                                      \
  if ((addr1 & 0xFF00) != (addr2 & 0xFF00)) {                                  \
    cycles++;                                                                  \
  }

#define SET_ZERO_NEGATIVE(value)                                               \
  cpu_set_flag(cpu, FLAG_ZERO, value == 0x00);                                 \
  cpu_set_flag(cpu, FLAG_NEGATIVE, value & 0x80);

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

static inline void adc(CPU *cpu, uint8_t data) {
  uint16_t result = cpu->a + data + (cpu->status & FLAG_CARRY);
  cpu_set_flag(cpu, FLAG_CARRY, result > 0xFF);
  cpu_set_flag(cpu, FLAG_ZERO, (result & 0x00FF) == 0);
  cpu_set_flag(cpu, FLAG_NEGATIVE, result & 0x80);
  cpu_set_flag(cpu, FLAG_OVERFLOW,
               (~(cpu->a ^ data) & (cpu->a ^ result)) & 0x80);
  cpu->a = result & 0x00FF;
}

static inline void asl(CPU *cpu, uint8_t *ptr) {
  uint16_t result = *ptr << 1;
  *ptr = result & 0x00FF;
  cpu_set_flag(cpu, FLAG_CARRY, result & 0xFF00);
  cpu_set_flag(cpu, FLAG_ZERO, *ptr == 0x00);
  cpu_set_flag(cpu, FLAG_NEGATIVE, *ptr & 0x80);
}

/**
 * Zero = A & data
 * status[6,7] = data[6,7]
 */
static inline void bit(CPU *cpu, uint8_t data) {
  cpu->status =
      (cpu->status & 0x3d) | (data & 0xc0) | ((data & cpu->a) == 0) << 1;
}

static inline void brk(CPU *cpu) {
  push(cpu, (cpu->pc >> 8) & 0x00FF);
  push(cpu, cpu->pc & 0x00FF);

  // push status with break set
  push(cpu, cpu->status | FLAG_BREAK);

  cpu_set_flag(cpu, FLAG_INTERRUPT_DISABLE, true);
  cpu->pc = bus_read_wide(cpu->bus, 0xFFFE, false);
}

static inline void cmp(CPU *cpu, uint8_t source, uint8_t data) {
  uint16_t result = source - data;
  cpu_set_flag(cpu, FLAG_NEGATIVE, result & 0x80);
  cpu_set_flag(cpu, FLAG_ZERO, result == 0);

  // TODO: understand why carry is <= 0xff
  cpu_set_flag(cpu, FLAG_CARRY, result <= 0xff);
}

static inline void inc(CPU *cpu, uint8_t *value, uint8_t ammount) {
  (*value) += ammount;
  cpu_set_flag(cpu, FLAG_NEGATIVE, *value & 0x80);
  cpu_set_flag(cpu, FLAG_ZERO, *value == 0);
}

static inline void eor(CPU *cpu, uint8_t data) {
  cpu->a ^= data;
  cpu_set_flag(cpu, FLAG_NEGATIVE, cpu->a & 0x80);
  cpu_set_flag(cpu, FLAG_ZERO, cpu->a == 0);
}

static inline void lsr(CPU *cpu, uint8_t *ptr) {
  cpu_set_flag(cpu, FLAG_CARRY, *ptr & 0x1);
  *ptr >>= 1;
  cpu_set_flag(cpu, FLAG_NEGATIVE, false);
  cpu_set_flag(cpu, FLAG_ZERO, *ptr == 0x00);
}

static inline void rol(CPU *cpu, uint8_t *ptr) {
  uint16_t result = *ptr << 1 | (cpu->status & 0x01);
  *ptr = result;
  cpu_set_flag(cpu, FLAG_CARRY, result & 0xFF00);
  cpu_set_flag(cpu, FLAG_ZERO, *ptr == 0x00);
  cpu_set_flag(cpu, FLAG_NEGATIVE, *ptr & 0x80);
}

static inline void ror(CPU *cpu, uint8_t *ptr) {
  uint8_t result = *ptr >> 1 | (cpu->status << 7);
  cpu_set_flag(cpu, FLAG_CARRY, *ptr & 0x1);
  *ptr = result;
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

void cpu_init(CPU *cpu, Bus *bus) {
  cpu->bus = bus;
  cpu_reset(cpu);
}

uint8_t cpu_step(CPU *cpu) {
  uint8_t opcode = bus_read(cpu->bus, cpu->pc, false);
  Instruction instruction = instructions[opcode];

  cpu->pc++;
  uint8_t cycles = instruction.cycles;
  uint8_t addressing_cycles = 0;
  uint8_t operation_cycles = 0;

  uint16_t addr;
  uint8_t data;
  uint8_t *ptr;

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

    // simulate page boundary bug
    if (ptr_lo == 0x00FF) {
      addr = (bus_read(cpu->bus, ptr & 0xFF00, false)) << 8 |
             (bus_read(cpu->bus, ptr, false));
    } else {
      addr = bus_read_wide(cpu->bus, ptr, false);
    }

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

  /* cpu_trace(instruction, cpu->pc, data); */

  switch (opcode) {
  case CASE8_4(0x61):
    adc(cpu, data);
    break;
  case CASE8_4(0x21):
    cpu->a &= data;
    cpu_set_flag(cpu, FLAG_ZERO, cpu->a == 0x00);
    cpu_set_flag(cpu, FLAG_NEGATIVE, cpu->a & 0x80);
    operation_cycles = 1;
    break;
  case CASE5(0x06):
    asl(cpu, ptr);
    break;
  case VERT_8(0x10):
    if ((cpu->status >> BRANCH_OFF[opcode >> 6] & 0x01) ==
        ((opcode >> 5) & 0x01)) {
      cycles++;
      addr = cpu->pc + addr;
      // TODO: sum instead of branch
      CHECK_PAGE_CROSSING(addr, cpu->pc);
      cpu->pc = addr;
    }
    break;
  // FIXME: f1 takes too long
  case CASE8_4(0xE1):
    adc(cpu, ~data);
    operation_cycles = 1;
    break;
  case 0x24:
  case 0x2c:
    bit(cpu, data);
    break;
  case 0x00:
    brk(cpu);
    break;
  /**          flag mask value
   * CLC 0x18   C   0x01  0
   * SEC 0x38   C   0x01  1
   * CLI 0x58   I   0x04  0
   * SEI 0x78   I   0x04  1
   * mask = 2 ** (opcode >> 6) * 2
   * value = bit 5 of opcode
   */
  case VERT_4(0x18):
    cpu_set_flag(cpu, 1 << ((opcode >> 6) << 1), opcode & 0x20);
    break;
  case CASE8_4(0xC1): // CMP
    cmp(cpu, cpu->a, data);
    operation_cycles = 1;
    break;
  case 0xE0:
  case 0xE4:
  case 0xEC:
    cmp(cpu, cpu->x, data);
    operation_cycles = 1;
    break;
  case 0xC0:
  case 0xC4:
  case 0xCC:
    cmp(cpu, cpu->y, data);
    operation_cycles = 1;
    break;
  case CASE4(0xC6):
    inc(cpu, ptr, -1);
    break;
  case 0xCA:
    inc(cpu, &cpu->x, -1);
    break;
  case 0x88:
    inc(cpu, &cpu->y, -1);
    break;
  case CASE8_4(0x41): // EOR
    eor(cpu, data);
    operation_cycles = 1;
    break;
  case CASE4(0xE6):
    inc(cpu, ptr, 1);
    break;
  case 0xE8:
    inc(cpu, &cpu->x, 1);
    break;
  case 0xC8:
    inc(cpu, &cpu->y, 1);
    break;
  case 0x4C:
  case 0x6C:
    cpu->pc = addr;
    break;
  case 0x20: // JSR
    push(cpu, (cpu->pc >> 8) & 0x00FF);
    push(cpu, cpu->pc & 0x00FF);
    cpu->pc = addr;
    break;
  case CASE8_4(0xA1): // LDX
    cpu->a = data;
    SET_ZERO_NEGATIVE(cpu->a);
    operation_cycles = 1;
    break;
  case CASE5(0xA2): // LDA
    cpu->x = data;
    SET_ZERO_NEGATIVE(cpu->x);
    operation_cycles = 1;
    break;
  case 0xA0:
  case CASE4(0xA4): // LDY
    cpu->y = data;
    SET_ZERO_NEGATIVE(cpu->y);
    operation_cycles = 1;
    break;
  case CASE5(0x46):
    lsr(cpu, ptr);
    break;
  case 0xEA:
    break;
  case CASE8_4(0x01): // ORA
    cpu->a |= data;
    SET_ZERO_NEGATIVE(cpu->a);
    operation_cycles = 1;
    break;
  case 0x48:
    push(cpu, cpu->a);
    break;
  case 0x08:
    push(cpu, cpu->status | FLAG_UNUSED | FLAG_BREAK);
    break;
  case 0x68: // PLA
    cpu->a = bus_read(cpu->bus, 0x0100 + ++cpu->sp, false);
    SET_ZERO_NEGATIVE(cpu->a);
    break;
  case 0x28: // PLP
    cpu->status = pop(cpu) & ~(FLAG_UNUSED | FLAG_BREAK);
    break;
  case CASE5(0x26):
    rol(cpu, ptr);
    break;
  case CASE5(0x66):
    ror(cpu, ptr);
    break;
  case 0x40:
    rti(cpu);
    break;
  case 0xB8:
    cpu_set_flag(cpu, FLAG_OVERFLOW, false);
    break;
  default:
    printf("Unimplemented opcode: %02X\n", opcode);
    exit(1);
    break;
  }

  cpu->cycles = cycles + (addressing_cycles & operation_cycles);
  return cpu->cycles;
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
