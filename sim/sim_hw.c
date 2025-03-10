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

    btn_value[BTN_STEP_1]  = IsKeyDown(KEY_A);
    btn_value[BTN_STEP_2]  = IsKeyDown(KEY_S);
    btn_value[BTN_STEP_3]  = IsKeyDown(KEY_D);
    btn_value[BTN_STEP_4]  = IsKeyDown(KEY_F);
    btn_value[BTN_STEP_5]  = IsKeyDown(KEY_G);
    btn_value[BTN_STEP_6]  = IsKeyDown(KEY_H);
    btn_value[BTN_STEP_7]  = IsKeyDown(KEY_J);
    btn_value[BTN_STEP_8]  = IsKeyDown(KEY_K);
    btn_value[BTN_STEP_9]  = IsKeyDown(161); // backslash \ on UK layout
    btn_value[BTN_STEP_10] = IsKeyDown(KEY_Z);
    btn_value[BTN_STEP_11] = IsKeyDown(KEY_X);
    btn_value[BTN_STEP_12] = IsKeyDown(KEY_C);
    btn_value[BTN_STEP_13] = IsKeyDown(KEY_V);
    btn_value[BTN_STEP_14] = IsKeyDown(KEY_B);
    btn_value[BTN_STEP_15] = IsKeyDown(KEY_N);
    btn_value[BTN_STEP_16] = IsKeyDown(KEY_M);

    btn_value[BTN_SHIFT]    = IsKeyDown(KEY_LEFT_SHIFT);
    btn_value[BTN_SOFT_A]   = IsKeyDown(KEY_O);
    btn_value[BTN_TRACK]    = IsKeyDown(KEY_ONE);
    btn_value[BTN_VOICE]    = IsKeyDown(KEY_TWO);
    btn_value[BTN_PATTERN]  = IsKeyDown(KEY_THREE);
    btn_value[BTN_LEFT]     = IsKeyDown(KEY_Q);
    btn_value[BTN_RIGHT]    = IsKeyDown(KEY_W);
    btn_value[BTN_SOFT_B]   = IsKeyDown(KEY_L);
    btn_value[BTN_PLAY]     = IsKeyDown(KEY_SPACE);
    btn_value[BTN_REC]      = IsKeyDown(KEY_R);
    //btn_value[BTN_UNKN2]
    btn_value[BTN_MENU]     = IsKeyDown(KEY_Y);
    btn_value[BTN_KEYBOARD] = IsKeyDown(KEY_T);

    int knob = 0;
    if (IsKeyDown(KEY_F2)) knob = 1;
    if (IsKeyDown(KEY_F3)) knob = 2;
    if (IsKeyDown(KEY_F4)) knob = 3;

    float wheel = GetMouseWheelMove(); //-1,0,1
    knob_position[knob] += wheel;

}



AudioBuffer get_audio_buffer(void) {
    AudioBuffer x;
    return x;
}


void put_audio_buffer(AudioBuffer buffer) {

}
