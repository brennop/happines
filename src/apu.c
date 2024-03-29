#include "apu.h"
#include <SDL2/SDL.h>

const uint8_t DUTY_CYCLE_TABLE[4][8] = {{0, 1, 0, 0, 0, 0, 0, 0},
                                        {0, 1, 1, 0, 0, 0, 0, 0},
                                        {0, 1, 1, 1, 1, 0, 0, 0},
                                        {1, 0, 0, 1, 1, 1, 1, 1}};

const uint8_t LENGTH_COUNTER_TABLE[32] = {10, 254, 20,  2,  40, 4,  80, 6,  160, 8,  60,
                                          10, 14,  12,  26, 14, 12, 16, 24, 18,  48, 20,
                                          96, 22,  192, 24, 72, 26, 16, 28, 32,  30};

const uint8_t TRIANGLE_TABLE[] = {0xF, 0xE, 0xD, 0xC, 0xB, 0xA, 0x9, 0x8, 0x7, 0x6, 0x5,
                                  0x4, 0x3, 0x2, 0x1, 0x0, 0x0, 0x1, 0x2, 0x3, 0x4, 0x5,
                                  0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF};

const uint16_t NOISE_PERIOD_TABLE[] = {4,   8,   16,  32,  64,  96,   128,  160,
                                       202, 254, 380, 508, 762, 1016, 2034, 4068};

const uint16_t DMC_PERIOD_TABLE[] = {428, 380, 340, 320, 286, 254, 226, 214,
                                     190, 160, 142, 128, 106, 84,  72,  54};

uint8_t pulse_output(Pulse *pulse) {
  bool value = DUTY_CYCLE_TABLE[pulse->duty_cycle][pulse->duty_value];
  if (!pulse->enabled || !value ||
      pulse->length_counter.value == 0
      // TODO: why timer_period?
      || pulse->timer_period < 8 || pulse->timer_period > 0x7FF) {
    return 0;
  }

  // envelope period is also used as volume
  return pulse->envelope.enabled ? pulse->envelope.value : pulse->envelope.period;
}

void pulse_step(Pulse *pulse) {
  if (pulse->timer) {
    pulse->timer--;
  } else {
    pulse->timer = pulse->timer_period;
    pulse->duty_value = (pulse->duty_value + 1) % 8;
  }
}

void pulse_sweep(Pulse *pulse) {
  uint16_t delta = pulse->timer_period >> pulse->sweep_shift;

  if (pulse->sweep_negate) {
    pulse->timer_period -= delta;
    if (pulse->channel == 1) pulse->timer_period--;
  } else {
    pulse->timer_period += delta;
  }
}

void pulse_sweep_step(Pulse *pulse) {
  if (pulse->sweep_reload) {
    if (pulse->sweep_enabled && pulse->sweep_counter == 0) {
      pulse_sweep(pulse);
    }

    pulse->sweep_counter = pulse->sweep_period;
    pulse->sweep_reload = false;
  } else if (pulse->sweep_counter > 0) {
    pulse->sweep_counter--;
  } else {
    if (pulse->sweep_enabled) pulse_sweep(pulse);

    pulse->sweep_counter = pulse->sweep_period;
  }
}

uint8_t triangle_output(Triangle *triangle) {
  if (!triangle->enabled || triangle->length_counter.value == 0 ||
      triangle->linear_counter_value == 0) {
    return 0;
  }

  return TRIANGLE_TABLE[triangle->duty_value];
}

void triangle_step(Triangle *triangle) {
  if (triangle->timer_value) {
    triangle->timer_value--;
  } else {
    triangle->timer_value = triangle->timer_period;
    triangle->duty_value = (triangle->duty_value + 1) % 32;
  }
}

void triangle_counter_step(Triangle *triangle) {
  // FIXME: is this realy needed? as it already happens in apu_write
  if (triangle->linear_counter_reload) {
    triangle->linear_counter_value = triangle->linear_counter_period;
  } else if (triangle->linear_counter_value > 0) {
    triangle->linear_counter_value--;
  }

  if (triangle->linear_counter_enabled) triangle->linear_counter_reload = false;
}

uint8_t noise_output(Noise *noise) {
  if (!noise->enabled || noise->length_counter.value == 0 || (noise->shift_register & 1) == 1) {
    return 0;
  }

  return noise->envelope.enabled ? noise->envelope.value : noise->envelope.period;
}

void noise_step(Noise *noise) {
  if (noise->timer_value == 0) {
    noise->timer_value = noise->timer_period;
    uint8_t feedback = noise->shift_register ^ (noise->shift_register >> (noise->mode ? 6 : 1));
    noise->shift_register >>= 1;
    noise->shift_register |= (feedback & 0x1) << 14;
  } else {
    noise->timer_value--;
  }
}

void dmc_reader_step(APU *apu) {
  DMC *dmc = &apu->dmc;
  if (dmc->current_length > 0 && dmc->bits_remaining == 0) {
    dmc->stall = true;

    dmc->shift_register = *apu->mapper->prg_read(apu->mapper, dmc->current_address);
    dmc->bits_remaining = 8;

    if (++dmc->current_address == 0) dmc->current_address = 0x8000;

    dmc->current_length--;

    if (dmc->current_length == 0) {
      if (dmc->loop) {
        dmc->current_length = dmc->sample_length;
        dmc->current_address = dmc->sample_address;
      } else {
        dmc->irq_active = true;
      }
    }
  }
}

void dmc_step(APU *apu) {
  DMC *dmc = &apu->dmc;
  if (!dmc->enabled) return;

  dmc_reader_step(apu);

  if (dmc->timer_value == 0) {
    dmc->timer_value = dmc->timer_period;

    if (dmc->bits_remaining > 0) {
      if (dmc->shift_register & 1) {
        if (dmc->value <= 125) dmc->value += 2;
      } else {
        if (dmc->value >= 2) dmc->value -= 2;
      }

      dmc->shift_register >>= 1;
      dmc->bits_remaining--;
    }
  } else {
    dmc->timer_value--;
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

static inline void step_envelopes(APU *apu) {
  envelope_step(&apu->pulses[0].envelope);
  envelope_step(&apu->pulses[1].envelope);
  envelope_step(&apu->noise.envelope);
  triangle_counter_step(&apu->triangle);
}

static inline void step_length_and_sweep(APU *apu) {
  length_counter_step(&apu->pulses[0].length_counter);
  length_counter_step(&apu->pulses[1].length_counter);
  length_counter_step(&apu->triangle.length_counter);
  length_counter_step(&apu->noise.length_counter);

  pulse_sweep_step(&apu->pulses[0]);
  pulse_sweep_step(&apu->pulses[1]);
}

void apu_init(APU *apu, Mapper *mapper) {
  apu->mapper = mapper;

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

  for (int i = 0; i < 203; i++) {
    apu->tnd_table[i] = 163.67 / (24329.0 / (float)i + 100);
  }

  apu->pulses[0].channel = 1;
  apu->pulses[1].channel = 2;

  apu->noise.shift_register = 1;

  apu->frame_step = 0;

  apu->frame_irq_active = false;
  apu->dmc.irq_active = false;
  apu->dmc.bits_remaining = 0;
}

void apu_write(APU *apu, uint16_t address, uint8_t value) {
  uint8_t channel = (address & 0x4) >> 2;
  switch (address) {
  case 0x4000:
  case 0x4004:
    apu->pulses[channel].duty_cycle = (value >> 6) & 0x03;
    apu->pulses[channel].length_counter.enabled = !(value & 0x20);
    apu->pulses[channel].envelope.enabled = !(value & 0x10);
    apu->pulses[channel].envelope.period = value & 0x0F;
    apu->pulses[channel].envelope.loop = (value & 0x20) > 0;
    apu->pulses[channel].envelope.start = true;
    break;
  case 0x4001:
  case 0x4005:
    apu->pulses[channel].sweep_enabled = (value & 0x80) > 0;
    apu->pulses[channel].sweep_period = ((value >> 4) & 0x07) + 1;
    apu->pulses[channel].sweep_negate = (value & 0x08) > 0;
    apu->pulses[channel].sweep_shift = value & 0x07;
    apu->pulses[channel].sweep_reload = true;
    break;
  case 0x4002:
  case 0x4006:
    apu->pulses[channel].timer_period = (apu->pulses[channel].timer_period & 0xFF00) | value;
    break;
  case 0x4003:
  case 0x4007:
    apu->pulses[channel].timer_period =
        (apu->pulses[channel].timer_period & 0x00FF) | ((uint16_t)(value & 0x07) << 8);
    apu->pulses[channel].duty_value = 0;
    apu->pulses[channel].length_counter.value = LENGTH_COUNTER_TABLE[value >> 3];
    apu->pulses[channel].envelope.start = true;
    break;
  case 0x4008:
    apu->triangle.length_counter.enabled = (value & 0x80) == 0;
    apu->triangle.linear_counter_enabled = (value & 0x80) == 0;
    apu->triangle.linear_counter_period = value & 0x7F;
    break;
  case 0x400A:
    apu->triangle.timer_period = (apu->triangle.timer_period & 0xFF00) | value;
    break;
  case 0x400B:
    apu->triangle.timer_period =
        (apu->triangle.timer_period & 0x00FF) | ((uint16_t)(value & 0x07) << 8);
    apu->triangle.length_counter.value = LENGTH_COUNTER_TABLE[value >> 3];
    // reload linear counter and set reload flag
    apu->triangle.linear_counter_value = apu->triangle.linear_counter_period;
    apu->triangle.linear_counter_reload = true;
    break;
  case 0x400C:
    apu->noise.length_counter.enabled = !(value & 0x20);
    apu->noise.envelope.enabled = !(value & 0x10);
    apu->noise.envelope.period = value & 0x0F;
    apu->noise.envelope.loop = (value & 0x20) > 0;
    apu->noise.envelope.start = true;
    break;
  case 0x400E:
    apu->noise.mode = value & 0x80;
    apu->noise.timer_period = NOISE_PERIOD_TABLE[value & 0x0F];
    break;
  case 0x400F:
    apu->noise.length_counter.value = LENGTH_COUNTER_TABLE[value >> 3];
    apu->noise.envelope.start = true;
    break;
  case 0x4010:
    apu->dmc.irq_enabled = value & 0x80;
    if (!apu->dmc.irq_enabled) apu->dmc.irq_active = false;
    apu->dmc.loop = value & 0x40;
    apu->dmc.timer_period = DMC_PERIOD_TABLE[value & 0x0F];
    break;
  case 0x4011: apu->dmc.load_counter = value & 0x7F; break;
  case 0x4012: apu->dmc.sample_address = 0xC000 | ((uint16_t)value << 6); break;
  case 0x4013: apu->dmc.sample_length = ((uint16_t)value << 4) | 0x0001; break;
  case 0x4015:
    apu->pulses[0].enabled = value & 0x01;
    if (!apu->pulses[0].enabled) apu->pulses[0].length_counter.value = 0;
    apu->pulses[1].enabled = value & 0x02;
    if (!apu->pulses[1].enabled) apu->pulses[1].length_counter.value = 0;
    apu->triangle.enabled = value & 0x04;
    if (!apu->triangle.enabled) apu->triangle.length_counter.value = 0;
    apu->noise.enabled = value & 0x08;
    if (!apu->noise.enabled) apu->noise.length_counter.value = 0;
    apu->dmc.enabled = value & 0x10;
    if (!apu->dmc.enabled)
      apu->dmc.current_length = 0;
    else if (apu->dmc.current_length == 0) {
      apu->dmc.current_length = apu->dmc.sample_length;
      apu->dmc.current_address = apu->dmc.sample_address;
      // TODO: check shift register
    }
    break;
  case 0x4017:
    apu->frame_counter = value;
    // clear interrupt flag
    if (apu->frame_counter & 0x40) apu->frame_irq_active = false;
    if (value == 0) apu->frame_irq_active = true;

    break;
  }
}

uint8_t apu_read(APU *apu, uint16_t address) {
  if (address == 0x4015) {
    apu->frame_irq_active = false;

    uint8_t value = 0;
    value |= apu->pulses[0].length_counter.value > 0 ? 0x01 : 0;
    value |= apu->pulses[1].length_counter.value > 0 ? 0x02 : 0;
    value |= apu->triangle.length_counter.value > 0 ? 0x04 : 0;
    value |= apu->noise.length_counter.value > 0 ? 0x08 : 0;
    value |= apu->dmc.current_length > 0 ? 0x10 : 0;
    value |= apu->frame_irq_active ? 0x40 : 0;
    value |= apu->dmc.irq_active ? 0x80 : 0;
    return value;
  }

  return 0;
}

void apu_step(APU *apu) {
  apu->cycles++;

  // clock timers
  if (apu->cycles % 2 == 0) {
    pulse_step(&apu->pulses[0]);
    pulse_step(&apu->pulses[1]);
    noise_step(&apu->noise);
    dmc_step(apu);
  }
  triangle_step(&apu->triangle);

  // clock length counters
  if (apu->cycles % (1789773 / 240) == 0) {
    if ((apu->frame_counter >> 7) & 1) { // 5-step
      switch (apu->frame_step % 5) {
      case 0: step_envelopes(apu); break;
      case 1:
        step_envelopes(apu);
        step_length_and_sweep(apu);
        break;
      case 2: step_envelopes(apu); break;
      case 3: /* do nothing */ break;
      case 4:
        step_envelopes(apu);
        step_length_and_sweep(apu);
        break;
      }
    } else { // 4-step
      step_envelopes(apu);
      if (apu->frame_step & 0x1) {
        step_length_and_sweep(apu);
      }

      if ((apu->frame_step & 0x3) == 3 && (apu->frame_counter & 0x40) == 0) {
        apu->frame_irq_active = true;
      }
    }

    apu->frame_step++;
  }

  // downsample
  if (apu->cycles % (APU_RATE / SAMPLE_RATE) / 2 == 0) {
    uint8_t p = pulse_output(&apu->pulses[0]) + pulse_output(&apu->pulses[1]);
    uint8_t t = triangle_output(&apu->triangle);
    uint8_t n = noise_output(&apu->noise);
    uint8_t d = apu->dmc.value;

    float sample = apu->pulse_table[p] + apu->tnd_table[3 * t + 2 * n + d];
    apu->buffer[apu->buffer_index++] = sample;
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
