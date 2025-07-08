#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "hw/perf.h"
#include <stdio.h>
#include "config.h"
#include "pico/stdlib.h"


/************************************************/
// Audio config

// Output sample rate
#define SAMPLE_RATE 48000

// Buffer size in samples
#define BUFFER_SIZE_SAMPS 256

/************************************************/

// 1 - 10
#define DEFAULT_BRIGHTNESS 3

// 0 - 100
#define DEFAULT_VOLUME 50



#define BUFFER_TIME_SEC ((BUFFER_SIZE_SAMPS) / (float)(SAMPLE_RATE))

// Show min and max sample values
//#define DEBUG_AMPLITUDE

//#define DEBUG_PRINTF(fmt, ...) (void)0
#define DEBUG_PRINTF printf

#define INIT_PRINTF printf


static inline void toggle_dbg_gpio() {
    const int pin = 18;
    gpio_put(pin, !gpio_get(pin));
}