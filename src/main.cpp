#include "common.h"
#include "hw/hw.h"
#include "hw/oled.h"
#include "input.h"
#include "synth_common.hpp"

#include "audio.hpp"
#include "userinterface.hpp"
#include "sample.hpp"

#include "debug_shell.h"

int main(void) {
    create_lookup_tables();
    hw_init();

    SampleManager::init();
    UI::init();
    hw_audio_start();

    // debug_shell_init();
    // prompt();

    bool update_display = true;
    int ctr = 0;
    while (1) {
        // Wait for audio generation to finish, get input data
        RawInput raw_input = audio_wait();

        // Update device state & draw UI
        if (UI::process(raw_input)) {
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

        if (++ctr == 256) {
            printf("perf:\taudio=%lld  cores=%lld,%lld\tui=%lld\n", 
                perf_get(PERF_AUDIO),
                perf_get(PERF_CHAN_CORE0),
                perf_get(PERF_CHAN_CORE1),
                perf_get(PERF_UI_UPDATE));
            ctr = 0;
        }
    }
}
