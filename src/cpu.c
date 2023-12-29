#include "cpu.h"
#include "instructions.h"

uint8_t cpu_fetch_data(CPU *cpu, AddressingMode addressing_mode) {
  switch (addressing_mode) {
  case ADDR_MODE_IMP:
    cpu->fetched = cpu->a;
    return 0;
  case ADDR_MODE_IMM:
    cpu->addr_abs = cpu->pc++;
    return 0;
  case ADDR_MODE_ZP0:
    cpu->addr_abs = bus_read(cpu->bus, cpu->pc++, false);
    cpu->addr_abs &= 0x00FF;
    return 0;
  case ADDR_MODE_ZPX:
    cpu->addr_abs = bus_read(cpu->bus, cpu->pc++, false) + cpu->x;
    cpu->addr_abs &= 0x00FF;
    return 0;
  case ADDR_MODE_ZPY:
    cpu->addr_abs = bus_read(cpu->bus, cpu->pc++, false) + cpu->y;
    cpu->addr_abs &= 0x00FF;
    return 0;
  case ADDR_MODE_ABS:
    cpu->addr_abs = bus_read(cpu->bus, cpu->pc++, false);
    cpu->addr_abs |= (bus_read(cpu->bus, cpu->pc++, false) << 8);
    return 0;
  case ADDR_MODE_ABX: {
    cpu->addr_abs = bus_read(cpu->bus, cpu->pc++, false);
    uint16_t addr_abs_hi = bus_read(cpu->bus, cpu->pc++, false);
    cpu->addr_abs |= (addr_abs_hi << 8);
    cpu->addr_abs += cpu->x;

    if ((cpu->addr_abs & 0xFF00) != (addr_abs_hi << 8)) {
      return 1;
    }
    return 0;
  }
  case ADDR_MODE_ABY: {
    cpu->addr_abs = bus_read(cpu->bus, cpu->pc++, false);
    uint16_t addr_abs_hi = bus_read(cpu->bus, cpu->pc++, false);
    cpu->addr_abs |= (addr_abs_hi << 8);
    cpu->addr_abs += cpu->y;

    if ((cpu->addr_abs & 0xFF00) != (addr_abs_hi << 8)) {
      return 1;
    }
    return 0;
  }
  case ADDR_MODE_IND: {
    uint16_t ptr_lo = bus_read(cpu->bus, cpu->pc++, false);
    uint16_t ptr_hi = bus_read(cpu->bus, cpu->pc++, false);
    uint16_t ptr = (ptr_hi << 8) | ptr_lo;

    cpu->addr_abs = bus_read_wide(cpu->bus, ptr, false);

    // TODO: check for page boundary crossing

    return 0;
  }
  case ADDR_MODE_IZX: {
    uint16_t t = bus_read(cpu->bus, cpu->pc++, false);
    uint16_t lo =
        bus_read(cpu->bus, (uint16_t)(t + (uint16_t)cpu->x) & 0x00FF, false);
    uint16_t hi = bus_read(
        cpu->bus, (uint16_t)(t + (uint16_t)cpu->x + 1) & 0x00FF, false);

    cpu->addr_abs = (hi << 8) | lo;

    return 0;
  }
  case ADDR_MODE_IZY: {
    uint16_t t = bus_read(cpu->bus, cpu->pc++, false);
    uint16_t lo = bus_read(cpu->bus, t & 0x00FF, false);
    uint16_t hi = bus_read(cpu->bus, (t + 1) & 0x00FF, false);
    cpu->addr_abs = (hi << 8) | lo;
    cpu->addr_abs += cpu->y;

    if ((cpu->addr_abs & 0xFF00) != (hi << 8)) {
      return 1;
    }
    return 0;
  }
  case ADDR_MODE_REL: {
    cpu->addr_rel = bus_read(cpu->bus, cpu->pc++, false);
    if (cpu->addr_rel & 0x80) {
      cpu->addr_rel |= 0xFF00;
    }
    return 0;
  }
  }
}

void cpu_init(CPU *cpu, Bus *bus) { cpu->bus = bus; }

void cpu_step(CPU *cpu) {
  uint8_t opcode = bus_read(cpu->bus, cpu->pc, false);
  Instruction instruction = instructions[opcode];

  cpu->pc++;
  uint8_t cycles = instruction.cycles;

  uint8_t memory_cycles = cpu_fetch_data(cpu, instruction.addressing_mode);

  cpu->cycles = cycles + memory_cycles;
}

void cpu_reset(CPU *cpu);
void cpu_irq(CPU *cpu);
void cpu_nmi(CPU *cpu);
