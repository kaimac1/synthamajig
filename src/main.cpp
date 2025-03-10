#include <stdio.h>

#include "common.h"
#include "hw/hw.h"
#include "hw/oled.h"
#include "gfx/ngl.h"
#include "input.h"
#include "synth_common.h"

#include "audio.hpp"
#include "ui.hpp"

// DMA transfer complete ISR
// - Read hardware inputs
// - Update parameters for current voice
// - Generate audio
extern "C" void audio_dma_callback(void) {
    RawInput input = input_read();
    AudioBuffer buffer = get_audio_buffer();
    audio_callback(buffer, input);
    put_audio_buffer(buffer);
}

int main() {
    create_lookup_tables();
    hw_init();
    ngl_init();
    ui_init();

    bool update_display = true;
    while (1) {
        // Wait for audio generation to finish, get input data
        RawInput raw_input = audio_wait();

        // Update device state & draw UI
        //time_loop_us = perf_loop(PERF_MAINLOOP);
        if (ui_process(raw_input)) {
            update_display = true;
        }

        // Write framebuffer out to display when needed
        if (update_display) {
            hw_debug_led(1);
            bool written = oled_write();
            if (written) {
                perf_loop(PERF_DISPLAY_UPDATE);
                update_display = false;
            }
            hw_debug_led(0);
        }
    }
}
