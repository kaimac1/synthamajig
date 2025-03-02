#include "../src/hw/hw.h"
#include "../src/hw/oled.h"
#include "../src/hw/pinmap.h"
#include "../src/gfx/ngl.h"
#include "../src/common.h"
#include <stdio.h>
#include "raylib.h"

uint8_t led_value[NUM_LEDS];
uint8_t btn_value[NUM_BUTTONS];

int knob_position[NUM_KNOBS] = {1,1,1,1};
uint16_t samplebuffer[512];

void oled_set_brightness(int brightness) {
    // not implemented
}

void hw_init(void) {
    //oled_init(ngl_framebuffer());
}


int32_t read_knob(int encoder) {
    if ((encoder < 0) || (encoder > 4)) return 0;
    return knob_position[encoder];
}


void hw_set_led(int led, uint8_t value) {
    int scaled = (value*value) >> 8;
    led_value[led] = scaled;
}


void hw_debug_led(bool value) {
    // not implemented
}


bool hw_read_button(int button) {
    return btn_value[button];
}


void delay_us_in_isr(uint32_t us) {
    // not implemented
}


void hw_scan_buttons(void) {

    btn_value[BTN_SHIFT]    = IsKeyPressed(KEY_LEFT_SHIFT);
    btn_value[BTN_SOFT_A]   = IsKeyPressed(KEY_O);
    btn_value[BTN_TRACK]    = IsKeyPressed(KEY_ONE);
    btn_value[BTN_VOICE]    = IsKeyPressed(KEY_TWO);
    btn_value[BTN_PATTERN]  = IsKeyPressed(KEY_THREE);
    btn_value[BTN_LEFT]     = IsKeyPressed(KEY_Q);
    btn_value[BTN_RIGHT]    = IsKeyPressed(KEY_W);
    btn_value[BTN_SOFT_B]   = IsKeyPressed(KEY_L);
    btn_value[BTN_PLAY]     = IsKeyPressed(KEY_SPACE);
    btn_value[BTN_REC]      = IsKeyPressed(KEY_R);
    //btn_value[BTN_UNKN2]
    btn_value[BTN_MENU]     = IsKeyPressed(KEY_M);
    btn_value[BTN_KEYBOARD] = IsKeyPressed(KEY_K);

    int knob = 0;
    if (IsKeyDown(KEY_F2)) knob = 1;
    if (IsKeyDown(KEY_F3)) knob = 2;
    if (IsKeyDown(KEY_F4)) knob = 3;

    float wheel = GetMouseWheelMove(); //-1,0,1
    knob_position[knob] += wheel;

}



AudioBuffer get_audio_buffer(void) {
    AudioBuffer buffer;
    buffer.samples = &samplebuffer[0];
    buffer.sample_count = 256;
    return buffer;
}


void put_audio_buffer(AudioBuffer buffer) {

}
