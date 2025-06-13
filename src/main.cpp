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

#include "pico/stdlib.h"

#include <cstdio>
#include "hw/psram_spi.h"
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

    gpio_init(PSRAM_PIN_CS0);
    gpio_set_dir(PSRAM_PIN_CS0, GPIO_OUT);
    gpio_put(PSRAM_PIN_CS0, 1);
    gpio_init(PSRAM_PIN_CS1);
    gpio_set_dir(PSRAM_PIN_CS1, GPIO_OUT);
    gpio_put(PSRAM_PIN_CS1, 1);

    debug_shell_init();
    prompt();

    while (1);
}



/*
    INIT_PRINTF("Initialising PSRAM...\n");
    psram_spi_inst_t psram = psram_spi_init(pio0);
    if (psram.error) {
        while (1);
    }
*/

