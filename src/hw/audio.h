/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once
#include "pico/audio_i2s.h"
#include "../common.h"

void fill_buffer(struct audio_buffer *buffer);

audio_format_t audio_format;

struct audio_buffer_pool *init_audio(uint32_t sample_rate, uint8_t pin_data, uint8_t pin_bclk, uint8_t pio_sm, uint8_t dma_ch) {
    audio_format.sample_freq = sample_rate;
    audio_format.format = AUDIO_BUFFER_FORMAT_PCM_S16;
    audio_format.channel_count = 1;

  static struct audio_buffer_format producer_format = {
    .format = &audio_format,
    .sample_stride = 2
  };

  struct audio_buffer_pool *producer_pool = audio_new_producer_pool(
    &producer_format,
    1,
    BUFFER_SIZE_SAMPS
  );

  const struct audio_format *output_format;

  struct audio_i2s_config config = {
    .data_pin = pin_data,
    .clock_pin_base = pin_bclk,
    .dma_channel = dma_ch,
    .pio_sm = pio_sm,
  };

  output_format = audio_i2s_setup(&audio_format, &config);
  if (!output_format) {
    panic("PicoAudio: Unable to open audio device.\n");
  }

  bool status = audio_i2s_connect(producer_pool);
  if (!status) {
    panic("PicoAudio: Unable to connect to audio device.\n");
  }

  return producer_pool;
}
