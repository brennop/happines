#include "apu.h"
#include <SDL2/SDL.h>

const uint8_t DUTY_CYCLE_TABLE[4][8] = {{0, 1, 0, 0, 0, 0, 0, 0},
                                        {0, 1, 1, 0, 0, 0, 0, 0},
                                        {0, 1, 1, 1, 1, 0, 0, 0},
                                        {1, 0, 0, 1, 1, 1, 1, 1}};

float sample(APU *apu, uint8_t pulse1) { return apu->pulse_table[pulse1]; }

uint8_t pulse_output(Pulse *pulse) {
  bool value = DUTY_CYCLE_TABLE[pulse->duty_cycle][pulse->duty];
  if (!pulse->enabled || !value) {
    return 0;
  }

  // TODO: why period?
  return pulse->envelope_enabled ? pulse->envelope_value
                                 : pulse->envelope_period;
}

void pulse_step(Pulse *pulse) {
  if (pulse->timer) {
    pulse->timer--;
  } else {
    pulse->timer = pulse->timer_period;
    pulse->duty = (pulse->duty + 1) % 8;
  }
}

void pulse_envelope_step(Pulse *pulse) {
  if (pulse->envelope_start) {
    pulse->envelope_start = false;
    pulse->envelope_value = 15;
    pulse->envelope_counter = pulse->envelope_period;
  } else {
    if (pulse->envelope_counter == 0) {
      // load divider with V
      pulse->envelope_counter = pulse->envelope_period;

      if (pulse->envelope_value > 0) {
        pulse->envelope_value--;
      } else if (pulse->envelope_loop) {
        pulse->envelope_value = 15;
      }
    } else {
      pulse->envelope_counter--;
    }
  }
}

void apu_init(APU *apu) {
  SDL_AudioSpec audio_spec;
  audio_spec.freq = 44100;
  audio_spec.format = AUDIO_F32SYS;
  audio_spec.channels = 2;
  audio_spec.samples = SAMPLES;
  audio_spec.callback = NULL;
  audio_spec.userdata = apu;

  SDL_AudioSpec obtained_spec;
  SDL_OpenAudio(&audio_spec, &obtained_spec);
  SDL_PauseAudio(0);

  apu->buffer_index = 0;
  for (int i = 0; i < SAMPLES; i++) {
    apu->buffer[i] = 0;
  }

  for (int i = 0; i < 31; i++) {
    apu->pulse_table[i] = 95.52 / (8128.0 / (float)i + 100);
  }
}

void apu_write(APU *apu, uint16_t address, uint8_t value) {
  switch (address) {
  case 0x4000:
    apu->pulse1.duty_cycle = (value >> 6) & 0x03;
    apu->pulse1.envelope_enabled = !(value & 0x10);
    apu->pulse1.envelope_period = value & 0x0F;
    break;
  case 0x4001:
    break;
  case 0x4002:
    apu->pulse1.timer_period = (apu->pulse1.timer_period & 0xFF00) | value;
    break;
  case 0x4003:
    apu->pulse1.timer_period =
        (apu->pulse1.timer_period & 0x00FF) | ((uint16_t)(value & 0x07) << 8);
    apu->pulse1.duty = 0;
    break;
  case 0x4015:
    apu->pulse1.enabled = value & 0x01;
    break;
  }
}

void apu_step(APU *apu) {
  apu->cycles++;

  // clock timers
  if (apu->cycles % 2 == 0) {
    pulse_step(&apu->pulse1);
  }

  // clock length counters
  if (apu->cycles % (1789773 / 240) == 0) {
    if ((apu->frame_counter >> 7) & 1) { // 5-step
      if (apu->frame_step % 5 != 3) {
        // clock envelope
        pulse_envelope_step(&apu->pulse1);
      }
    } else { // 4-step
      pulse_envelope_step(&apu->pulse1);
    }

    apu->frame_step++;
  }

  // downsample
  if (apu->cycles % (APU_RATE / SAMPLE_RATE) == 0) {
    apu->buffer[apu->buffer_index++] = sample(apu, pulse_output(&apu->pulse1));
  }

  if (apu->buffer_index >= SAMPLES) {
    apu->buffer_index = 0;
    SDL_QueueAudio(1, apu->buffer, SAMPLES * sizeof(float));
  }
}
