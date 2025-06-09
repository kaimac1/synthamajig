#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "hw/perf.h"
#include <stdio.h>

#include "pico/stdlib.h"

#define SAMPLE_RATE 44100

// Show min and max sample values
//#define DEBUG_AMPLITUDE

//#define DEBUG_PRINTF(fmt, ...) (void)0
#define DEBUG_PRINTF printf

#define INIT_PRINTF printf


static inline void toggle_dbg_gpio() {
    const int pin = 18;
    gpio_put(pin, !gpio_get(pin));
}