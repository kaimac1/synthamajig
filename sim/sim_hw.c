#include "../src/hw/hw.h"
#include "../src/hw/oled.h"
#include "../src/hw/pinmap.h"
#include "../src/gfx/ngl.h"
#include "../src/common.h"

uint8_t led_value[NUM_LEDS];
uint8_t btn_value[NUM_BUTTONS];

void oled_set_brightness(int brightness) {
    //
}

void hw_init(void) {

    //oled_init(ngl_framebuffer());

}


int32_t read_knob(int encoder) {
    if ((encoder < 0) || (encoder > 4)) return 0;
    return 1;
}


void hw_set_led(int led, uint8_t value) {
    int scaled = (value*value) >> 8;
    led_value[led] = scaled;
}


void hw_debug_led(bool value) {

}


bool hw_read_button(int button) {
    return btn_value[button];
}


void delay_us_in_isr(uint32_t us) {
    //int64_t now = get_absolute_time();
    //while (get_absolute_time() - now < us);
}


void hw_scan_buttons(void) {

    // // Stop scanning and blank LEDs while reading the buttons
    // cancel_repeating_timer(&led_timer);
    // gpio_init(PIN_LED0);
    // gpio_init(PIN_LED1);

    // for (int col=0; col<NUM_COLUMNS; col++) {
    //     set_column(col);
    //     sleep_us(2);
    //     //delay_us_in_isr(2); // we should need this but don't for some reason
    //     btn_value[col+8*0] = !gpio_get(PIN_BTN0);
    //     btn_value[col+8*1] = !gpio_get(PIN_BTN1);
    //     btn_value[col+8*2] = !gpio_get(PIN_BTN2);
    //     btn_value[col+8*3] = !gpio_get(PIN_BTN3);
    // }

    // set_column(led_column);
    // led_timer_start();
}


AudioBuffer get_audio_buffer(void) {
    AudioBuffer buffer;
    buffer.samples = NULL;
    buffer.sample_count = 0;
    return buffer;
}


void put_audio_buffer(AudioBuffer buffer) {

}
