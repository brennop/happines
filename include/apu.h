#ifndef __APU_H__
#define __APU_H__

#include "common.h"

#define SAMPLES 512
#define SAMPLE_RATE 44100
#define APU_RATE 1789773

typedef struct {
  bool enabled;
  bool loop;
  bool start;
  uint8_t period;
  uint8_t value;
  uint8_t counter;
} Envelope;

typedef struct {
  bool enabled;
  uint8_t value;
} LengthCounter;

typedef struct {
  bool enabled;

  uint8_t duty_cycle;
  uint8_t duty_value;

  uint16_t timer;
  uint16_t timer_period;

  Envelope envelope;
  LengthCounter length_counter;

  bool sweep_reload;
  bool sweep_enabled;
  bool sweep_negate;
  uint8_t sweep_period;
  uint8_t sweep_counter;
  uint8_t sweep_shift;

  uint8_t channel;
} Pulse;

typedef struct {
  bool enabled;
  LengthCounter length_counter;

  bool linear_counter_enabled;
  uint8_t linear_counter_period;
  uint16_t timer_period;

  bool linear_counter_reload;
  uint8_t linear_counter_value;
  uint8_t duty_value;
  uint16_t timer_value;
} Triangle;

typedef struct {
  Pulse pulses[2];
  Triangle triangle;

  float pulse_table[31];
  float tnd_table[203];

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
