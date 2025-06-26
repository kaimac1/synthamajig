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

    //SampleManager::init();
    UI::init();
    hw_audio_start();

    //debug_shell_init();
    //prompt();

    INIT_PRINTF("main loop\n");
    int cnt = 0;
    while (1) {
        RawInput raw_input = audio_wait();
        UI::process(raw_input);

        if (cnt++ % 256 == 0) INIT_PRINTF(".");
    }
}
