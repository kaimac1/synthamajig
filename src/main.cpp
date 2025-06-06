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

    sleep_ms(2000);

    puts("Initing PSRAM...");
    psram_spi_inst_t psram = psram_spi_init(pio0, 0);
    if (psram.error) {
        while (1);
    }

    uint32_t psram_begin, psram_elapsed;
    float psram_speed;

    puts("Testing PSRAM...");

    // **************** 32 bits testing ****************
    psram_begin = time_us_32();
    for (uint32_t addr = 0; addr < (8 * 1024 * 1024); addr += 4) {
        psram_write32(&psram, addr, addr + 10);
    }
    psram_elapsed = time_us_32() - psram_begin;
    psram_speed = 1000000.0 * 8 / psram_elapsed;
    printf("32 bit: PSRAM write 8MB in %d us, %.2f MB/s\n", psram_elapsed, psram_speed);

    psram_begin = time_us_32();
    for (uint32_t addr = 0; addr < (8 * 1024 * 1024); addr += 4) {
        uint32_t result = psram_read32(&psram, addr);
        if (result != addr+10) {
            printf("PSRAM failure at address %x (%08x != %08x) ", addr, result, addr+10);
            while (1);
        }
    }
    psram_elapsed = (time_us_32() - psram_begin);
    psram_speed = 1000000.0 * 8 / psram_elapsed;
    printf("32 bit: PSRAM read 8MB in %d us, %.2f MB/s\n", psram_elapsed, psram_speed);    


    psram_begin = time_us_32();
    const int bufsiz = 4;
    const int nbytes = 4*bufsiz;
    for (uint32_t addr = 0; addr < 8*1024*1024; addr += nbytes) {
        uint32_t buffer[bufsiz];
        psram_readwords(&psram, addr, buffer, bufsiz);
        if (buffer[2] != addr+18) {
            printf("PSRAM failure at address %x (%08x %08x %08x %08x) ", addr, buffer[0], buffer[1], buffer[2], buffer[3]);
            while (1);
        }
    }
    psram_elapsed = (time_us_32() - psram_begin);
    psram_speed = 1000000.0 * 8 / psram_elapsed;
    printf("%d byte buffer: PSRAM read 8MB in %d us, %.2f MB/s\n", nbytes, psram_elapsed, psram_speed);    


    printf("done.\n");
    while (1) {
        hw_debug_led(1);
        sleep_ms(500);
        hw_debug_led(0);
        sleep_ms(500);
    }





    // // **************** n bits testing ****************
    // uint8_t write_data[256];
    // for (size_t i = 0; i < 256; ++i) {
    //     write_data[i] = i;
    // }
    // psram_begin = time_us_32();
    // for (uint32_t addr = 0; addr < (8 * 1024 * 1024); addr += 256) {
    //     for (uint32_t step = 0; step < 256; step += 16) {
    //         psram_write(&psram_spi, addr + step, write_data + step, 16);
    //     }
    // }
    // psram_elapsed = time_us_32() - psram_begin;
    // psram_speed = 1000000.0 * 8 / psram_elapsed;
    // printf("128 bit: PSRAM write 8MB in %d us, %.2f MB/s\n", psram_elapsed, psram_speed);

    // psram_begin = time_us_32();
    // uint8_t read_data[16];
    // for (uint32_t addr = 0; addr < (8 * 1024 * 1024); addr += 256) {
    //     for (uint32_t step = 0; step < 256; step += 16) {
    //         psram_read(&psram_spi, addr + step, read_data, 16);
    //         if (memcmp(read_data, write_data + step, 16) != 0) {
    //             printf("PSRAM failure at address %x", addr);
    //             return 1;
    //         }
    //     }
    // }
    // psram_elapsed = time_us_32() - psram_begin;
    // psram_speed = 1000000.0 * 8 / psram_elapsed;
    // printf("128 bit: PSRAM read 8MB in %d us, %.2f MB/s\n", psram_elapsed, psram_speed);

}
