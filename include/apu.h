#ifndef __APU_H__
#define __APU_H__

#include "common.h"

#define SAMPLES 1024
#define SAMPLE_RATE 44100
#define APU_RATE 1789773

typedef struct {
  bool enabled;

  uint8_t duty_cycle;
  /** index into duty cycle table */
  uint8_t duty;

  uint16_t timer;
  uint16_t timer_period;

  // envelope
  bool envelope_enabled;
  bool envelope_loop;
  bool envelope_start;
  uint8_t envelope_period;
  uint8_t envelope_value;
  uint8_t envelope_counter;
} Pulse;

typedef struct {
  Pulse pulse1;

  float pulse_table[31];

  float buffer[SAMPLES];
  uint32_t buffer_index;

  uint32_t cycles;
  uint32_t frame_step;
  uint8_t frame_counter;
} APU;

void apu_init(APU *apu);
void apu_step(APU *apu);
void apu_write(APU *apu, uint16_t address, uint8_t value);

#endif // __APU_H__
