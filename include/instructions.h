#ifndef __INSTRUCTIONS_H__
#define __INSTRUCTIONS_H__

#include "cpu.h"
#include <stdint.h>

typedef enum {
  ADDR_MODE_IMP, // Implied addressing mode
  ADDR_MODE_IMM, // Immediate addressing mode
  ADDR_MODE_ZP0, // Zero Page addressing mode
  ADDR_MODE_ZPX, // Zero Page with X Index addressing mode
  ADDR_MODE_ZPY, // Zero Page with Y Index addressing mode
  ADDR_MODE_REL, // Relative addressing mode
  ADDR_MODE_ABS, // Absolute addressing mode
  ADDR_MODE_ABX, // Absolute with X Index addressing mode
  ADDR_MODE_ABY, // Absolute with Y Index addressing mode
  ADDR_MODE_IND, // Indirect addressing mode
  ADDR_MODE_IZX, // Indirect Zero Page with X Index addressing mode
  ADDR_MODE_IZY  // Indirect Zero Page with Y Index addressing mode
} AddressingMode;

typedef struct {
  char *mnemonic;
  AddressingMode addressing_mode;
  uint8_t cycles;
} Instruction;

Instruction instructions[] = {
    {"BRK", ADDR_MODE_IMM, 7}, {"ORA", ADDR_MODE_IZX, 6},
    {"???", ADDR_MODE_IMP, 2}, {"???", ADDR_MODE_IMP, 8},
    {"???", ADDR_MODE_IMP, 3}, {"ORA", ADDR_MODE_ZP0, 3},
    {"ASL", ADDR_MODE_ZP0, 5}, {"???", ADDR_MODE_IMP, 5},
    {"PHP", ADDR_MODE_IMP, 3}, {"ORA", ADDR_MODE_IMM, 2},
    {"ASL", ADDR_MODE_IMP, 2}, {"???", ADDR_MODE_IMP, 2},
    {"???", ADDR_MODE_IMP, 4}, {"ORA", ADDR_MODE_ABS, 4},
    {"ASL", ADDR_MODE_ABS, 6}, {"???", ADDR_MODE_IMP, 6},
    {"BPL", ADDR_MODE_REL, 2}, {"ORA", ADDR_MODE_IZY, 5},
    {"???", ADDR_MODE_IMP, 2}, {"???", ADDR_MODE_IMP, 8},
    {"???", ADDR_MODE_IMP, 4}, {"ORA", ADDR_MODE_ZPX, 4},
    {"ASL", ADDR_MODE_ZPX, 6}, {"???", ADDR_MODE_IMP, 6},
    {"CLC", ADDR_MODE_IMP, 2}, {"ORA", ADDR_MODE_ABY, 4},
    {"???", ADDR_MODE_IMP, 2}, {"???", ADDR_MODE_IMP, 7},
    {"???", ADDR_MODE_IMP, 4}, {"ORA", ADDR_MODE_ABX, 4},
    {"ASL", ADDR_MODE_ABX, 7}, {"???", ADDR_MODE_IMP, 7},
    {"JSR", ADDR_MODE_ABS, 6}, {"AND", ADDR_MODE_IZX, 6},
    {"???", ADDR_MODE_IMP, 2}, {"???", ADDR_MODE_IMP, 8},
    {"BIT", ADDR_MODE_ZP0, 3}, {"AND", ADDR_MODE_ZP0, 3},
    {"ROL", ADDR_MODE_ZP0, 5}, {"???", ADDR_MODE_IMP, 5},
    {"PLP", ADDR_MODE_IMP, 4}, {"AND", ADDR_MODE_IMM, 2},
    {"ROL", ADDR_MODE_IMP, 2}, {"???", ADDR_MODE_IMP, 2},
    {"BIT", ADDR_MODE_ABS, 4}, {"AND", ADDR_MODE_ABS, 4},
    {"ROL", ADDR_MODE_ABS, 6}, {"???", ADDR_MODE_IMP, 6},
    {"BMI", ADDR_MODE_REL, 2}, {"AND", ADDR_MODE_IZY, 5},
    {"???", ADDR_MODE_IMP, 2}, {"???", ADDR_MODE_IMP, 8},
    {"???", ADDR_MODE_IMP, 4}, {"AND", ADDR_MODE_ZPX, 4},
    {"ROL", ADDR_MODE_ZPX, 6}, {"???", ADDR_MODE_IMP, 6},
    {"SEC", ADDR_MODE_IMP, 2}, {"AND", ADDR_MODE_ABY, 4},
    {"???", ADDR_MODE_IMP, 2}, {"???", ADDR_MODE_IMP, 7},
    {"???", ADDR_MODE_IMP, 4}, {"AND", ADDR_MODE_ABX, 4},
    {"ROL", ADDR_MODE_ABX, 7}, {"???", ADDR_MODE_IMP, 7},
    {"RTI", ADDR_MODE_IMP, 6}, {"EOR", ADDR_MODE_IZX, 6},
    {"???", ADDR_MODE_IMP, 2}, {"???", ADDR_MODE_IMP, 8},
    {"???", ADDR_MODE_IMP, 3}, {"EOR", ADDR_MODE_ZP0, 3},
    {"LSR", ADDR_MODE_ZP0, 5}, {"???", ADDR_MODE_IMP, 5},
    {"PHA", ADDR_MODE_IMP, 3}, {"EOR", ADDR_MODE_IMM, 2},
    {"LSR", ADDR_MODE_IMP, 2}, {"???", ADDR_MODE_IMP, 2},
    {"JMP", ADDR_MODE_ABS, 3}, {"EOR", ADDR_MODE_ABS, 4},
    {"LSR", ADDR_MODE_ABS, 6}, {"???", ADDR_MODE_IMP, 6},
    {"BVC", ADDR_MODE_REL, 2}, {"EOR", ADDR_MODE_IZY, 5},
    {"???", ADDR_MODE_IMP, 2}, {"???", ADDR_MODE_IMP, 8},
    {"???", ADDR_MODE_IMP, 4}, {"EOR", ADDR_MODE_ZPX, 4},
    {"LSR", ADDR_MODE_ZPX, 6}, {"???", ADDR_MODE_IMP, 6},
    {"CLI", ADDR_MODE_IMP, 2}, {"EOR", ADDR_MODE_ABY, 4},
    {"???", ADDR_MODE_IMP, 2}, {"???", ADDR_MODE_IMP, 7},
    {"???", ADDR_MODE_IMP, 4}, {"EOR", ADDR_MODE_ABX, 4},
    {"LSR", ADDR_MODE_ABX, 7}, {"???", ADDR_MODE_IMP, 7},
    {"RTS", ADDR_MODE_IMP, 6}, {"ADC", ADDR_MODE_IZX, 6},
    {"???", ADDR_MODE_IMP, 2}, {"???", ADDR_MODE_IMP, 8},
    {"???", ADDR_MODE_IMP, 3}, {"ADC", ADDR_MODE_ZP0, 3},
    {"ROR", ADDR_MODE_ZP0, 5}, {"???", ADDR_MODE_IMP, 5},
    {"PLA", ADDR_MODE_IMP, 4}, {"ADC", ADDR_MODE_IMM, 2},
    {"ROR", ADDR_MODE_IMP, 2}, {"???", ADDR_MODE_IMP, 2},
    {"JMP", ADDR_MODE_IND, 5}, {"ADC", ADDR_MODE_ABS, 4},
    {"ROR", ADDR_MODE_ABS, 6}, {"???", ADDR_MODE_IMP, 6},
    {"BVS", ADDR_MODE_REL, 2}, {"ADC", ADDR_MODE_IZY, 5},
    {"???", ADDR_MODE_IMP, 2}, {"???", ADDR_MODE_IMP, 8},
    {"???", ADDR_MODE_IMP, 4}, {"ADC", ADDR_MODE_ZPX, 4},
    {"ROR", ADDR_MODE_ZPX, 6}, {"???", ADDR_MODE_IMP, 6},
    {"SEI", ADDR_MODE_IMP, 2}, {"ADC", ADDR_MODE_ABY, 4},
    {"???", ADDR_MODE_IMP, 2}, {"???", ADDR_MODE_IMP, 7},
    {"???", ADDR_MODE_IMP, 4}, {"ADC", ADDR_MODE_ABX, 4},
    {"ROR", ADDR_MODE_ABX, 7}, {"???", ADDR_MODE_IMP, 7},
    {"???", ADDR_MODE_IMP, 2}, {"STA", ADDR_MODE_IZX, 6},
    {"???", ADDR_MODE_IMP, 2}, {"???", ADDR_MODE_IMP, 6},
    {"STY", ADDR_MODE_ZP0, 3}, {"STA", ADDR_MODE_ZP0, 3},
    {"STX", ADDR_MODE_ZP0, 3}, {"???", ADDR_MODE_IMP, 3},
    {"DEY", ADDR_MODE_IMP, 2}, {"???", ADDR_MODE_IMP, 2},
    {"TXA", ADDR_MODE_IMP, 2}, {"???", ADDR_MODE_IMP, 2},
    {"STY", ADDR_MODE_ABS, 4}, {"STA", ADDR_MODE_ABS, 4},
    {"STX", ADDR_MODE_ABS, 4}, {"???", ADDR_MODE_IMP, 4},
    {"BCC", ADDR_MODE_REL, 2}, {"STA", ADDR_MODE_IZY, 6},
    {"???", ADDR_MODE_IMP, 2}, {"???", ADDR_MODE_IMP, 6},
    {"STY", ADDR_MODE_ZPX, 4}, {"STA", ADDR_MODE_ZPX, 4},
    {"STX", ADDR_MODE_ZPY, 4}, {"???", ADDR_MODE_IMP, 4},
    {"TYA", ADDR_MODE_IMP, 2}, {"STA", ADDR_MODE_ABY, 5},
    {"TXS", ADDR_MODE_IMP, 2}, {"???", ADDR_MODE_IMP, 5},
    {"???", ADDR_MODE_IMP, 5}, {"STA", ADDR_MODE_ABX, 5},
    {"???", ADDR_MODE_IMP, 5}, {"???", ADDR_MODE_IMP, 5},
    {"LDY", ADDR_MODE_IMM, 2}, {"LDA", ADDR_MODE_IZX, 6},
    {"LDX", ADDR_MODE_IMM, 2}, {"???", ADDR_MODE_IMP, 6},
    {"LDY", ADDR_MODE_ZP0, 3}, {"LDA", ADDR_MODE_ZP0, 3},
    {"LDX", ADDR_MODE_ZP0, 3}, {"???", ADDR_MODE_IMP, 3},
    {"TAY", ADDR_MODE_IMP, 2}, {"LDA", ADDR_MODE_IMM, 2},
    {"TAX", ADDR_MODE_IMP, 2}, {"???", ADDR_MODE_IMP, 2},
    {"LDY", ADDR_MODE_ABS, 4}, {"LDA", ADDR_MODE_ABS, 4},
    {"LDX", ADDR_MODE_ABS, 4}, {"???", ADDR_MODE_IMP, 4},
    {"BCS", ADDR_MODE_REL, 2}, {"LDA", ADDR_MODE_IZY, 5},
    {"???", ADDR_MODE_IMP, 2}, {"???", ADDR_MODE_IMP, 5},
    {"LDY", ADDR_MODE_ZPX, 4}, {"LDA", ADDR_MODE_ZPX, 4},
    {"LDX", ADDR_MODE_ZPY, 4}, {"???", ADDR_MODE_IMP, 4},
    {"CLV", ADDR_MODE_IMP, 2}, {"LDA", ADDR_MODE_ABY, 4},
    {"TSX", ADDR_MODE_IMP, 2}, {"???", ADDR_MODE_IMP, 4},
    {"LDY", ADDR_MODE_ABX, 4}, {"LDA", ADDR_MODE_ABX, 4},
    {"LDX", ADDR_MODE_ABY, 4}, {"???", ADDR_MODE_IMP, 4},
    {"CPY", ADDR_MODE_IMM, 2}, {"CMP", ADDR_MODE_IZX, 6},
    {"???", ADDR_MODE_IMP, 2}, {"???", ADDR_MODE_IMP, 8},
    {"CPY", ADDR_MODE_ZP0, 3}, {"CMP", ADDR_MODE_ZP0, 3},
    {"DEC", ADDR_MODE_ZP0, 5}, {"???", ADDR_MODE_IMP, 5},
    {"INY", ADDR_MODE_IMP, 2}, {"CMP", ADDR_MODE_IMM, 2},
    {"DEX", ADDR_MODE_IMP, 2}, {"???", ADDR_MODE_IMP, 2},
    {"CPY", ADDR_MODE_ABS, 4}, {"CMP", ADDR_MODE_ABS, 4},
    {"DEC", ADDR_MODE_ABS, 6}, {"???", ADDR_MODE_IMP, 6},
    {"BNE", ADDR_MODE_REL, 2}, {"CMP", ADDR_MODE_IZY, 5},
    {"???", ADDR_MODE_IMP, 2}, {"???", ADDR_MODE_IMP, 8},
    {"???", ADDR_MODE_IMP, 4}, {"CMP", ADDR_MODE_ZPX, 4},
    {"DEC", ADDR_MODE_ZPX, 6}, {"???", ADDR_MODE_IMP, 6},
    {"CLD", ADDR_MODE_IMP, 2}, {"CMP", ADDR_MODE_ABY, 4},
    {"NOP", ADDR_MODE_IMP, 2}, {"???", ADDR_MODE_IMP, 7},
    {"???", ADDR_MODE_IMP, 4}, {"CMP", ADDR_MODE_ABX, 4},
    {"DEC", ADDR_MODE_ABX, 7}, {"???", ADDR_MODE_IMP, 7},
    {"CPX", ADDR_MODE_IMM, 2}, {"SBC", ADDR_MODE_IZX, 6},
    {"???", ADDR_MODE_IMP, 2}, {"???", ADDR_MODE_IMP, 8},
    {"CPX", ADDR_MODE_ZP0, 3}, {"SBC", ADDR_MODE_ZP0, 3},
    {"INC", ADDR_MODE_ZP0, 5}, {"???", ADDR_MODE_IMP, 5},
    {"INX", ADDR_MODE_IMP, 2}, {"SBC", ADDR_MODE_IMM, 2},
    {"NOP", ADDR_MODE_IMP, 2}, {"???", ADDR_MODE_IMP, 2},
    {"CPX", ADDR_MODE_ABS, 4}, {"SBC", ADDR_MODE_ABS, 4},
    {"INC", ADDR_MODE_ABS, 6}, {"???", ADDR_MODE_IMP, 6},
    {"BEQ", ADDR_MODE_REL, 2}, {"SBC", ADDR_MODE_IZY, 5},
    {"???", ADDR_MODE_IMP, 2}, {"???", ADDR_MODE_IMP, 8},
    {"???", ADDR_MODE_IMP, 4}, {"SBC", ADDR_MODE_ZPX, 4},
    {"INC", ADDR_MODE_ZPX, 6}, {"???", ADDR_MODE_IMP, 6},
    {"SED", ADDR_MODE_IMP, 2}, {"SBC", ADDR_MODE_ABY, 4},
    {"NOP", ADDR_MODE_IMP, 2}, {"???", ADDR_MODE_IMP, 7},
    {"???", ADDR_MODE_IMP, 4}, {"SBC", ADDR_MODE_ABX, 4},
    {"INC", ADDR_MODE_ABX, 7}, {"???", ADDR_MODE_IMP, 7},
};

#endif // __INSTRUCTIONS_H__
