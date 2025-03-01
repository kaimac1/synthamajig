#include <stdio.h>

#include "common.h"
#include "hw/hw.h"
#include "hw/oled.h"
#include "gfx/ngl.h"
#include "synth_common.h"
#include "input.h"

#include "ui.hpp"
#include "track.hpp"
#include "instrument.hpp"

struct {
    volatile bool audio_done;
    Input input;
} shared;

extern Track track;

// The "audio context" where all of our synthesis happens.
// - Read hardware inputs (encoders, buttons)
// - Update parameters for currently selected voice based on those inputs
// - Generate a new buffer of samples
extern "C" void audio_dma_callback(void) {
    static Input input_prev;

    perf_start(PERF_AUDIO);
    Input input = input_get();
    shared.input = input;

    if (input_detect_events(&input, input_prev)) {
        input_prev = input;
        // Change parameters via encoders
        track.control_active_voice(&input);

        play_notes_from_input(&input);
    }

    // Sample generation
    AudioBuffer buffer = get_audio_buffer();
    track.fill_buffer(buffer);
    put_audio_buffer(buffer);

    // delay_us_in_isr(58*70); // simulated cpu usage
    perf_end(PERF_AUDIO);
    shared.audio_done = 1;
}

int main() {
    create_lookup_tables();
    hw_init();
    ngl_init();
    ui_init();

    bool update_display = true;
    while (1) {
        // Wait for end of audio task
        while (!shared.audio_done);
        shared.audio_done = 0;
        // If we are here then the audio task has just completed, so we have
        // plenty of time to read shared.input safely
        // TODO: redo this with proper locking just to be sure

        //time_loop_us = perf_loop(PERF_MAINLOOP);

        if (ui_process(shared.input)) {
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
