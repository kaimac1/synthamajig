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
    hw_init();

    debug_shell_init();
    prompt();

    while (1);
}
