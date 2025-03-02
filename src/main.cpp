#include <stdio.h>

#include "common.h"
#include "hw/hw.h"
#include "hw/oled.h"
#include "gfx/ngl.h"
#include "input.h"
#include "synth_common.h"

#include "audio.hpp"
#include "ui.hpp"

int main() {
    create_lookup_tables();
    hw_init();
    ngl_init();
    ui_init();

    bool update_display = true;
    while (1) {
        Input input = audio_wait();

        //time_loop_us = perf_loop(PERF_MAINLOOP);

        if (ui_process(input)) {
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
