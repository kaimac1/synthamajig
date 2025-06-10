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

#include "nandflash/nand_spi.h"
#include "nandflash/nandflash.h"


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

    gpio_init(PIN_NAND_CS);
    gpio_set_dir(PIN_NAND_CS, GPIO_OUT);
    gpio_put(PIN_NAND_HOLD, 1);    

    sleep_ms(2000);

    INIT_PRINTF("Initialising PSRAM...\n");
    psram_spi_inst_t psram = psram_spi_init(pio0);
    if (psram.error) {
        while (1);
    }

    nand_spi_init();
    int r = nandflash_init(NULL);
    INIT_PRINTF("  init returned %d\n", r);

    uint8_t buffer[256];
    row_address_t row;
    row.whole = 0;
    r = nandflash_page_read(row, 0, buffer, sizeof(buffer));
    INIT_PRINTF("read=%d\n",r);
    INIT_PRINTF("buffer= %02x %02x %02x %02x\n", buffer[0], buffer[1], buffer[2], buffer[3]);    

    // uint8_t wbuffer[4] = {1, 2, 3, 4};
    // r = nandflash_page_program(row, 0, wbuffer, sizeof(wbuffer));
    // INIT_PRINTF("write=%d\n",r);    

    for (uint32_t block=0; block<1024; block++) {
        row_address_t row = {.page = 0, .block = block};
        bool is_bad;
        r = nandflash_block_is_bad(row, &is_bad);
        INIT_PRINTF("block %04d r=%d bad=%d\n", row.block, r, is_bad);
    }



    // r = nandflash_block_erase(row);
    // INIT_PRINTF("erase=%d\n",r);


    // r = nandflash_page_read(row, 0, buffer, sizeof(buffer));
    // INIT_PRINTF("read=%d\n",r);
    // INIT_PRINTF("buffer= %02x %02x %02x %02x\n", buffer[0], buffer[1], buffer[2], buffer[3]);



    printf("done.\n");
    while (1) {
        hw_debug_led(1);
        sleep_ms(500);
        hw_debug_led(0);
        sleep_ms(500);
    }
}
