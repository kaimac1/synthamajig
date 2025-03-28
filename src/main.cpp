#include "common.h"
#include "hw/hw.h"
#include "hw/oled.h"
#include "input.h"
#include "synth_common.hpp"

#include "audio.hpp"
#include "userinterface.hpp"
#include "sample.hpp"


#include "pico/stdlib.h"
#include "ff.h"
#include "hw/pinmap.h"
#include <cstdio>
#include "tlsf/tlsf.h"


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

    SampleManager::init();

    perf_start(PERF_TEST);
    SampleManager::load(0);
    int64_t tt = perf_end(PERF_TEST);
    printf("Load took %lld us\n", tt);

    UI ui;
    ui.init();

    hw_audio_start();

    bool update_display = true;
    int ctr = 0;
    while (1) {
        // Wait for audio generation to finish, get input data
        RawInput raw_input = audio_wait();

        // Update device state & draw UI
        //time_loop_us = perf_loop(PERF_MAINLOOP);
        perf_start(PERF_UI_UPDATE);
        if (ui.process(raw_input)) {
            update_display = true;
        }
        int64_t time_ui = perf_end(PERF_UI_UPDATE);
        if (ctr % 256 == 0) {
            printf("time ui: %lld us\n", time_ui);
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

        ctr++;
    }
}
