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
#include "hardware/spi.h"
#include "hw/psram_spi.h"


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
    hw_init();

    gpio_init(PSRAM_PIN_CS0);
    gpio_set_dir(PSRAM_PIN_CS0, GPIO_OUT);
    gpio_put(PSRAM_PIN_CS0, 1);
    gpio_init(PSRAM_PIN_CS1);
    gpio_set_dir(PSRAM_PIN_CS1, GPIO_OUT);
    gpio_put(PSRAM_PIN_CS1, 1);


    sleep_ms(2000);

    puts("Initing PSRAM...");
    psram_spi_inst_t psram = psram_spi_init(pio0);
    if (psram.error) {
        while (1);
    }

 

    printf("done.\n");
    while (1) {
        hw_debug_led(1);
        sleep_ms(500);
        hw_debug_led(0);
        sleep_ms(500);
    }
}
