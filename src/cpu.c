#include "cpu.h"
#include "instructions.h"

static inline void cpu_set_flag(CPU *cpu, uint8_t offset, bool value) {
  cpu->status |= (value << offset);
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
    addr = bus_read(cpu->bus, cpu->pc++, false) |
           (bus_read(cpu->bus, cpu->pc++, false) << 8);
    break;
  case ADDR_MODE_ABX: {
    addr = bus_read(cpu->bus, cpu->pc++, false);
    uint16_t addr_abs_hi = bus_read(cpu->bus, cpu->pc++, false);
    addr |= (addr_abs_hi << 8);
    addr += cpu->x;

    if ((addr & 0xFF00) != (addr_abs_hi << 8))
      addressing_cycles++;
  }
  case ADDR_MODE_ABY: {
    addr = bus_read(cpu->bus, cpu->pc++, false);
    uint16_t addr_abs_hi = bus_read(cpu->bus, cpu->pc++, false);
    addr |= (addr_abs_hi << 8);
    addr += cpu->y;

    if ((addr & 0xFF00) != (addr_abs_hi << 8))
      addressing_cycles++;
  }
  case ADDR_MODE_IND: {
    uint16_t ptr_lo = bus_read(cpu->bus, cpu->pc++, false);
    uint16_t ptr_hi = bus_read(cpu->bus, cpu->pc++, false);
    uint16_t ptr = (ptr_hi << 8) | ptr_lo;

    addr = bus_read_wide(cpu->bus, ptr, false);

    // TODO: check for page boundary crossing
  }
  case ADDR_MODE_IZX: {
    uint16_t t = bus_read(cpu->bus, cpu->pc++, false);
    uint16_t lo =
        bus_read(cpu->bus, (uint16_t)(t + (uint16_t)cpu->x) & 0x00FF, false);
    uint16_t hi = bus_read(
        cpu->bus, (uint16_t)(t + (uint16_t)cpu->x + 1) & 0x00FF, false);

    addr = (hi << 8) | lo;
  }
  case ADDR_MODE_IZY: {
    uint16_t t = bus_read(cpu->bus, cpu->pc++, false);
    uint16_t lo = bus_read(cpu->bus, t & 0x00FF, false);
    uint16_t hi = bus_read(cpu->bus, (t + 1) & 0x00FF, false);
    addr = (hi << 8) | lo;
    addr += cpu->y;

    if ((addr & 0xFF00) != (hi << 8))
      addressing_cycles++;
  }
  case ADDR_MODE_REL: {
    addr = bus_read(cpu->bus, cpu->pc++, false);
    if (addr & 0x80) {
      addr |= 0xFF00;
    }
  }
  }

  if (instruction.addressing_mode != ADDR_MODE_IMP) {
    data = bus_read(cpu->bus, addr, false);
  }

  switch (opcode) {
  // TODO: Use a macro to generate this
  case 0x21: // AND (Indirect,X)
    cpu->a &= data;
    cpu_set_flag(cpu, FLAG_ZERO, cpu->a == 0x00);
    cpu_set_flag(cpu, FLAG_NEGATIVE, cpu->a & 0x80);
    operation_cycles = 1;
  }

  cpu->cycles = cycles + addressing_cycles & operation_cycles;
}

void cpu_reset(CPU *cpu);
void cpu_irq(CPU *cpu);
void cpu_nmi(CPU *cpu);
