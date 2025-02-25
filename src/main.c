#include <stdio.h>
#include "pico/stdlib.h"
#include "hw/hw.h"
#include "gfx/ngl.h"
#include "hw/oled.h"


void audio_dma_callback(void) {

    gpio_put(PICO_LED_PIN, 1);

    struct audio_buffer *buffer = get_audio_buffer();
    int16_t *samples = (int16_t *) buffer->buffer->bytes;
    int count = buffer->max_sample_count;
    const int volume = 1024;
    static int32_t sample = 0;

    for (int sn=0; sn<count; sn++) {

        sample += 32;
        sample %= 2048;
        
        int16_t samp = sample;
        
        samples[sn] = samp;
    }

    buffer->sample_count = buffer->max_sample_count;

    delay_us_in_isr(500);
    put_audio_buffer(buffer);
    gpio_put(PICO_LED_PIN, 0);

}


int main() {
    hw_init();
    build_font_index();

    draw_text(16, 16, 0, "Hello?");
    oled_write();



    bool led = true;
    while (true) {
        hw_scan_buttons();
        hw_set_led(4, led*255);
        printf("Hello, world %d!\n", read_knob(0));
        sleep_ms(1000);
        led = !led;
    }
}