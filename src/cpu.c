#include "cpu.h"
#include "instructions.h"

#include <stdio.h>
#include <stdlib.h>

const uint8_t BRANCH_OFF[] = {7, 6, 0, 1};

void cpu_trace(Instruction instruction, CPU* cpu, uint8_t op, uint16_t addr, uint16_t addr2) {
  printf("0x%04X: %02X %s 0x%04X, A: %02X X: %02X Y: %02X\n", addr, op, instruction.mnemonic, addr2, cpu->a, cpu->x, cpu->y);
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
  cpu_set_flag(cpu, FLAG_NEGATIVE, (value)&0x80);

bool is_opcode_legal(uint8_t opcode) {
  return instructions[opcode].mnemonic[0] != '?';
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

static inline uint8_t asl(CPU *cpu, uint8_t data) {
  uint16_t result = data << 1;
  cpu_set_flag(cpu, FLAG_CARRY, result & 0xFF00);
  cpu_set_flag(cpu, FLAG_ZERO, (result & 0x00FF) == 0);
  cpu_set_flag(cpu, FLAG_NEGATIVE, result & 0x80);

  return result & 0x00FF;
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

static inline void eor(CPU *cpu, uint8_t data) {
  cpu->a ^= data;
  cpu_set_flag(cpu, FLAG_NEGATIVE, cpu->a & 0x80);
  cpu_set_flag(cpu, FLAG_ZERO, cpu->a == 0);
}

static inline uint8_t lsr(CPU *cpu, uint8_t data) {
  uint8_t result = data >> 1;
  cpu_set_flag(cpu, FLAG_CARRY, data & 0x1);
  cpu_set_flag(cpu, FLAG_NEGATIVE, false);
  cpu_set_flag(cpu, FLAG_ZERO, result == 0x00);
  return result;
}

static inline uint8_t rol(CPU *cpu, uint8_t data) {
  uint16_t result = data << 1 | (cpu->status & 0x01);
  cpu_set_flag(cpu, FLAG_CARRY, result & 0xFF00);
  cpu_set_flag(cpu, FLAG_ZERO, (result & 0x00FF) == 0x00);
  cpu_set_flag(cpu, FLAG_NEGATIVE, result & 0x80);
  return result & 0x00FF;
}

static inline uint8_t ror(CPU *cpu, uint8_t data) {
  uint8_t result = data >> 1 | (cpu->status << 7);
  cpu_set_flag(cpu, FLAG_CARRY, data & 0x1);
  cpu_set_flag(cpu, FLAG_ZERO, result == 0x00);
  cpu_set_flag(cpu, FLAG_NEGATIVE, result & 0x80);
  return result;
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

// FIXME: improve this
// this is a hack to prevent from changing the ppu
#define DATA()                                                                 \
  (instruction.addressing_mode ? bus_read(cpu->bus, addr, false) : cpu->a)

uint8_t cpu_step(CPU *cpu) {
  uint8_t opcode = bus_read(cpu->bus, cpu->pc, false);
  Instruction instruction = instructions[opcode];

  uint16_t trace_pc = cpu->pc;

  // FIXME: hack to make compiler happy about unused variables
  cpu->pc = trace_pc + 1;

  uint8_t cycles = instruction.cycles;
  uint8_t addressing_cycles = 0;
  uint8_t operation_cycles = 0;

  uint16_t addr = 0;

  // Addressing modes
  switch (instruction.addressing_mode) {
  case ADDR_MODE_IMP:
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

  /* cpu_trace(instruction, cpu, opcode, trace_pc, addr); */

  switch (opcode) {
  case 0xFA:
  case 0xDA: // NOP
    break;
  case CASE8_4(0x61):
    adc(cpu, DATA());
    operation_cycles = 1;
    break;
  case CASE8_4(0x21):
    cpu->a &= DATA();
    cpu_set_flag(cpu, FLAG_ZERO, cpu->a == 0x00);
    cpu_set_flag(cpu, FLAG_NEGATIVE, cpu->a & 0x80);
    operation_cycles = 1;
    break;
  case 0x0A:
    cpu->a = asl(cpu, cpu->a);
    break;
  case CASE4(0x06):
    bus_write(cpu->bus, addr, asl(cpu, DATA()));
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
    adc(cpu, ~DATA());
    operation_cycles = 1;
    break;
  case 0x24:
  case 0x2c:
    bit(cpu, DATA());
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
  case 0xB8:
    cpu_set_flag(cpu, FLAG_OVERFLOW, false);
    break;
  case 0xD8:
    cpu_set_flag(cpu, FLAG_DECIMAL_MODE, false);
    break;
  case 0xF8:
    cpu_set_flag(cpu, FLAG_DECIMAL_MODE, true);
    break;
  case CASE8_4(0xC1): // CMP
    cmp(cpu, cpu->a, DATA());
    operation_cycles = 1;
    break;
  case 0xE0:
  case 0xE4:
  case 0xEC:
    cmp(cpu, cpu->x, DATA());
    operation_cycles = 1;
    break;
  case 0xC0:
  case 0xC4:
  case 0xCC:
    cmp(cpu, cpu->y, DATA());
    operation_cycles = 1;
    break;
  case CASE4(0xC6): {
    uint8_t data = bus_read(cpu->bus, addr, false);
    bus_write(cpu->bus, addr, data - 1);
    SET_ZERO_NEGATIVE(data - 1);
    break;
  }
  case 0xCA:
    cpu->x--;
    SET_ZERO_NEGATIVE(cpu->x);
    break;
  case 0x88:
    cpu->y--;
    SET_ZERO_NEGATIVE(cpu->y);
    break;
  case CASE8_4(0x41): // EOR
    eor(cpu, DATA());
    operation_cycles = 1;
    break;
  case CASE4(0xE6): { // INC
    uint8_t data = bus_read(cpu->bus, addr, false);
    bus_write(cpu->bus, addr, data + 1);
    SET_ZERO_NEGATIVE((uint8_t)(data + 1));
    break;
  }
  case 0xE8:
    cpu->x++;
    SET_ZERO_NEGATIVE(cpu->x);
    break;
  case 0xC8:
    cpu->y++;
    SET_ZERO_NEGATIVE(cpu->y);
    break;
  case 0x4C:
  case 0x6C:
    cpu->pc = addr;
    break;
  case 0x20: // JSR
    cpu->pc--;
    push(cpu, (cpu->pc >> 8) & 0x00FF);
    push(cpu, cpu->pc & 0x00FF);
    cpu->pc = addr;
    break;
  case CASE8_4(0xA1): // LDA
    cpu->a = DATA();
    SET_ZERO_NEGATIVE(cpu->a);
    operation_cycles = 1;
    break;
  case 0xA2:
  case CASE4(0xA6): // LDX
    cpu->x = DATA();
    SET_ZERO_NEGATIVE(cpu->x);
    operation_cycles = 1;
    break;
  case 0xA0:
  case CASE4(0xA4): // LDY
    cpu->y = DATA();
    SET_ZERO_NEGATIVE(cpu->y);
    operation_cycles = 1;
    break;
  case CASE4(0x46):
    bus_write(cpu->bus, addr, lsr(cpu, DATA()));
    break;
  case 0x4A:
    cpu->a = lsr(cpu, cpu->a);
    break;
  case 0xEA:
    break;
  case CASE8_4(0x01): // ORA
    cpu->a |= DATA();
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
  case CASE4(0x26):
    bus_write(cpu->bus, addr, rol(cpu, DATA()));
    break;
  case 0x2A:
    cpu->a = rol(cpu, cpu->a);
    break;
  case CASE4(0x66):
    bus_write(cpu->bus, addr, ror(cpu, DATA()));
    break;
  case 0x6A:
    cpu->a = ror(cpu, cpu->a);
    break;
  case 0x40:
    rti(cpu);
    break;
  case 0x60:
    cpu->pc = (pop(cpu) | ((uint16_t)pop(cpu) << 8)) + 1;
    break;
  case CASE8_4(0x81):
    bus_write(cpu->bus, addr, cpu->a);
    break;
  case CASE4(0x86):
    bus_write(cpu->bus, addr, cpu->x);
    break;
  case CASE4(0x84):
    bus_write(cpu->bus, addr, cpu->y);
    break;
  case 0xAA:
    cpu->x = cpu->a;
    SET_ZERO_NEGATIVE(cpu->x);
    break;
  case 0xA8:
    cpu->y = cpu->a;
    SET_ZERO_NEGATIVE(cpu->y);
    break;
  case 0x8A:
    cpu->a = cpu->x;
    SET_ZERO_NEGATIVE(cpu->a);
    break;
  case 0x98:
    cpu->a = cpu->y;
    SET_ZERO_NEGATIVE(cpu->a);
    break;
  case 0xBA:
    cpu->x = cpu->sp;
    SET_ZERO_NEGATIVE(cpu->x);
    break;
  case 0x9A:
    cpu->sp = cpu->x;
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
