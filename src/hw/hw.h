#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define NUM_KNOBS 4
#define NUM_LEDS 16
#define NUM_BUTTONS 32

#define LED_OFF 0
#define LED_DIM 64
#define LED_ON  255

typedef enum {
    BTN_STEP_1 = 0,
    BTN_STEP_2,
    BTN_STEP_3,
    BTN_STEP_4,
    BTN_STEP_5,
    BTN_STEP_6,
    BTN_STEP_7,
    BTN_STEP_8,
    BTN_STEP_9,
    BTN_STEP_10,
    BTN_STEP_11,
    BTN_STEP_12,
    BTN_STEP_13,
    BTN_STEP_14,
    BTN_STEP_15,
    BTN_STEP_16,
    BTN_SHIFT = 16,
    // 17 absent
    BTN_SOFT_A = 18,
    BTN_TRACK = 19,
    BTN_VOICE = 20,
    BTN_PATTERN = 21,
    // 22,23 absent
    BTN_LEFT = 24,
    BTN_RIGHT = 25,
    BTN_SOFT_B = 26,
    BTN_PLAY = 27,
    BTN_REC = 28,
     BTN_UNKN2 = 29,
    BTN_MENU = 30,
    BTN_KEYBOARD = 31
} ButtonName;

typedef struct {
    uint16_t *samples;
    uint32_t sample_count;
} AudioBuffer;


void hw_init(void);

// Read one of the rotary encoders. Absolute (cumulative) value, can be positive or negative.
int32_t read_knob(int encoder);

// Scan the input matrix
void hw_scan_buttons(void);

bool hw_read_button(int button);

void hw_set_led(int led, uint8_t value);

void hw_debug_led(bool value);

// Microsecond delay which can be used in an interrupt service routine
void delay_us_in_isr(uint32_t us);

// Get audio buffer - blocks until one is available
AudioBuffer get_audio_buffer(void);

// Send out a previously got buffer
void put_audio_buffer(AudioBuffer buffer);

#ifdef __cplusplus
}
#endif
