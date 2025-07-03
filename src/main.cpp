#include "common.h"
#include "hw/hw.h"
#include "hw/oled.h"
#include "hw/disk.h"
#include "hw/pinmap.h"
#include "input.h"
#include "synth_common.hpp"

#include "audio.hpp"
#include "userinterface.hpp"
#include "sample.hpp"

#include "debug_shell.h"


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

int main(void) {
    create_lookup_tables();
    hw_init();

    SampleManager::init();
    UI::init();
    hw_audio_start();

    //debug_shell_init();
    //prompt();

    bool update_display = true;
    int ctr = 0;
    while (1) {
        // Wait for audio generation to finish, get input data
        RawInput raw_input = audio_wait();

        // Update device state & draw UI
        //time_loop_us = perf_loop(PERF_MAINLOOP);
        perf_start(PERF_UI_UPDATE);
        if (UI::process(raw_input)) {
            update_display = true;
        }
        int64_t time_ui = perf_end(PERF_UI_UPDATE);
        if (time_ui > 500) {
            //printf("time ui: %lld us\n", time_ui);
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
