#ifndef __PPU_H__
#define __PPU_H__

#include "common.h"

typedef struct {
  uint8_t nametable[2][1024];
  uint8_t palette[32];


} PPU;

#endif // __PPU_H__
