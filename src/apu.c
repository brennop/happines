#include "apu.h"
#include <SDL2/SDL.h>

const uint8_t DUTY_CYCLE_TABLE[4][8] = {{0, 1, 0, 0, 0, 0, 0, 0},
                                        {0, 1, 1, 0, 0, 0, 0, 0},
                                        {0, 1, 1, 1, 1, 0, 0, 0},
                                        {1, 0, 0, 1, 1, 1, 1, 1}};

const uint8_t LENGTH_COUNTER_TABLE[32] = {
    10, 254, 20, 2,  40, 4,  80, 6,  160, 8,  60, 10, 14, 12, 26, 14,
    12, 16,  24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30};

float sample(APU *apu, uint8_t pulse1) { return apu->pulse_table[pulse1]; }

uint8_t pulse_output(Pulse *pulse) {
  bool value = DUTY_CYCLE_TABLE[pulse->duty_cycle][pulse->duty_value];
  if (!pulse->enabled || !value ||
      pulse->length_counter.value == 0
      // TODO: why timer_period?
      || pulse->timer_period < 8 || pulse->timer_period > 0x7FF) {
    return 0;
  }

  // TODO: why period?
  return pulse->envelope.enabled ? pulse->envelope.value
                                 : pulse->envelope.period;
}

void pulse_step(Pulse *pulse) {
  if (pulse->timer) {
    pulse->timer--;
  } else {
    pulse->timer = pulse->timer_period;
    pulse->duty_value = (pulse->duty_value + 1) % 8;
  }
}

void pulse_sweep_step(Pulse *pulse) {
  if (pulse->sweep_reload) {
    if (pulse->sweep_enabled && pulse->sweep_counter == 0) {
      // TODO
    }

    pulse->sweep_counter = pulse->sweep_period;
    pulse->sweep_reload = false;
  } else if (pulse->sweep_counter > 0) {
    pulse->sweep_counter--;
  } else {
    if (pulse->sweep_enabled) {
      uint16_t delta = pulse->timer_period >> pulse->sweep_shift;

      if (pulse->sweep_negate) {
        pulse->timer_period -= delta;
        if (pulse->channel == 1) {
          pulse->timer_period--;
        }
      } else {
        pulse->timer_period += delta;
      }
    }

    pulse->sweep_counter = pulse->sweep_period;
  }
}

void envelope_step(Envelope *envelope) {
  if (envelope->start) {
    envelope->start = false;
    envelope->value = 15;
    envelope->counter = envelope->period;
  } else {
    if (envelope->counter == 0) {
      // load divider with V
      envelope->counter = envelope->period;

      if (envelope->value > 0) {
        envelope->value--;
      } else if (envelope->loop) {
        envelope->value = 15;
      }
    } else {
      envelope->counter--;
    }
  }
}

void length_counter_step(LengthCounter *length_counter) {
  if (length_counter->enabled && length_counter->value > 0) {
    length_counter->value--;
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
    apu->pulse1.length_counter.enabled = !(value & 0x20);
    apu->pulse1.envelope.enabled = !(value & 0x10);
    apu->pulse1.envelope.period = value & 0x0F;
    apu->pulse1.envelope.loop = (value & 0x20) > 0;
    apu->pulse1.envelope.start = true;
    break;
  case 0x4001:
    apu->pulse1.sweep_enabled = value & 0x80;
    apu->pulse1.sweep_period = ((value >> 4) & 0x07) + 1;
    apu->pulse1.sweep_negate = value & 0x08;
    apu->pulse1.sweep_shift = value & 0x07;
    apu->pulse1.sweep_reload = true;
    break;
  case 0x4002:
    apu->pulse1.timer_period = (apu->pulse1.timer_period & 0xFF00) | value;
    break;
  case 0x4003:
    apu->pulse1.timer_period =
        (apu->pulse1.timer_period & 0x00FF) | ((uint16_t)(value & 0x07) << 8);
    apu->pulse1.duty_value = 0;
    apu->pulse1.length_counter.value = LENGTH_COUNTER_TABLE[value >> 3];
    apu->pulse1.envelope.start = true;
    break;
  case 0x4015:
    apu->pulse1.enabled = value & 0x01;
    if (!apu->pulse1.enabled)
      apu->pulse1.length_counter.value = 0;
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
      switch (apu->frame_step % 5) {
      case 0:
        envelope_step(&apu->pulse1.envelope);
        break;
      case 1:
        envelope_step(&apu->pulse1.envelope);
        length_counter_step(&apu->pulse1.length_counter);
        pulse_sweep_step(&apu->pulse1);
      case 2:
        envelope_step(&apu->pulse1.envelope);
        break;
      case 3:
        /* do nothing */
        break;
      case 4:
        envelope_step(&apu->pulse1.envelope);
        length_counter_step(&apu->pulse1.length_counter);
        pulse_sweep_step(&apu->pulse1);
        break;
      }
    } else { // 4-step
      envelope_step(&apu->pulse1.envelope);
      if (apu->frame_step & 0x1) {
        length_counter_step(&apu->pulse1.length_counter);
        pulse_sweep_step(&apu->pulse1);
      }
    }

    apu->frame_step++;
  }

  // downsample
  if (apu->cycles % (APU_RATE / SAMPLE_RATE) == 0) {
    apu->buffer[apu->buffer_index++] = sample(apu, pulse_output(&apu->pulse1));
  }

  if (apu->buffer_index >= SAMPLES) {
    apu->buffer_index = 0;
    // sync
    while (SDL_GetQueuedAudioSize(1) > SAMPLES * sizeof(float)) {
      SDL_Delay(1);
    }
    SDL_QueueAudio(1, apu->buffer, SAMPLES * sizeof(float));
  }
}
