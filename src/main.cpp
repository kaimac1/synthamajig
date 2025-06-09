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
    int r = spi_nand_init(NULL);
    INIT_PRINTF("  init returned %d\n", r);

    // puts("Initialising NAND disk...");
    // gpio_init(PIN_NAND_WP);
    // gpio_set_dir(PIN_NAND_WP, GPIO_OUT);
    // gpio_put(PIN_NAND_WP, 1);
    // gpio_init(PIN_NAND_HOLD);
    // gpio_set_dir(PIN_NAND_HOLD, GPIO_OUT);
    // gpio_put(PIN_NAND_HOLD, 1);

    // const int disk_clock_speed = 1000*1000;
    // spi_init(NAND_SPI, disk_clock_speed);
    // //gpio_set_function(PIN_NAND_CS, GPIO_FUNC_SPI);
    // gpio_set_function(PIN_NAND_SCK, GPIO_FUNC_SPI);
    // gpio_set_function(PIN_NAND_MOSI, GPIO_FUNC_SPI);
    // gpio_set_function(PIN_NAND_MISO, GPIO_FUNC_SPI);

    // const uint8_t cmd_reset = 0xFF;
    // gpio_put(PIN_NAND_CS, 0);
    // spi_write_blocking(NAND_SPI, &cmd_reset, 1);
    // gpio_put(PIN_NAND_CS, 1);
    // sleep_ms(1);

    // const uint8_t cmd_read_id[] = {0x9F, 0x00};
    // uint8_t buffer_read_id[3];
    // gpio_put(PIN_NAND_CS, 0);
    // spi_write_blocking(NAND_SPI, cmd_read_id, sizeof(cmd_read_id));
    // spi_read_blocking(NAND_SPI, 0x00, buffer_read_id, sizeof(buffer_read_id));
    // gpio_put(PIN_NAND_CS, 1);

    // printf("Read ID: %02x %02x %02x\n", buffer_read_id[0], buffer_read_id[1], buffer_read_id[2]);
 

    printf("done.\n");
    while (1) {
        hw_debug_led(1);
        sleep_ms(500);
        hw_debug_led(0);
        sleep_ms(500);
    }
}
