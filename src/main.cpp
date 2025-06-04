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

#define PIN_SPI_MISO    0
#define PIN_SPI_CS      1
#define PIN_SPI_SCK     2
#define PIN_SPI_MOSI    3


void psram_cmd(uint8_t cmd) {
    gpio_put(PIN_SPI_CS, 0);
    spi_write_blocking(spi0, &cmd, 1);
    gpio_put(PIN_SPI_CS, 1);
}

void psram_write(uint32_t addr, uint8_t *data, size_t len) {
    gpio_put(PIN_SPI_CS, 0);
    uint8_t cmd[] = {0x02, // write
                    (uint8_t)((addr >> 16) & 0xFF), 
                    (uint8_t)((addr >> 8) & 0xFF),
                    (uint8_t)(addr & 0xFF)
                    };
    spi_write_blocking(spi0, cmd, sizeof(cmd));
    spi_write_blocking(spi0, data, len);
    gpio_put(PIN_SPI_CS, 1);
}

void psram_read(uint32_t addr, uint8_t *buffer, size_t len) {
    gpio_put(PIN_SPI_CS, 0);
    uint8_t cmd[] = {0x03, // read (slow)
                    (uint8_t)((addr >> 16) & 0xFF), 
                    (uint8_t)((addr >> 8) & 0xFF),
                    (uint8_t)(addr & 0xFF)
                    };
    spi_write_blocking(spi0, cmd, sizeof(cmd));    
    spi_read_blocking(spi0, 0x00, buffer, len);
    gpio_put(PIN_SPI_CS, 1);
}


void write_test_1(size_t num) {
    printf("writing %d bytes...\n", num);
    perf_start(PERF_TEST);
    for (uint32_t addr=0; addr<num; addr++) {
        uint8_t data = addr & 0xFF;
        psram_write(addr, &data, 1);
    }
    int64_t tt = perf_end(PERF_TEST);
    float tt_sec = tt / 1E6;
    float mb_per_sec = ((float)num / 1048576) / tt_sec;
    printf("wrote %d bytes in %.2f sec (%.2f MB/sec)\n", num, tt_sec, mb_per_sec);
}

void write_test_4(size_t num) {
    printf("writing %d bytes...\n", num);
    perf_start(PERF_TEST);
    for (uint32_t addr=0; addr<num; addr+=4) {
        uint8_t data[4] = {
            (uint8_t)(addr & 0xFF),
            (uint8_t)((addr + 1) & 0xFF),
            (uint8_t)((addr + 2) & 0xFF),
            (uint8_t)((addr + 3) & 0xFF)
        };
        psram_write(addr, data, 4);
    }
    int64_t tt = perf_end(PERF_TEST);
    float tt_sec = tt / 1E6;
    float mb_per_sec = ((float)num / 1048576) / tt_sec;
    printf("wrote %d bytes in %.2f sec (%.2f MB/sec)\n", num, tt_sec, mb_per_sec);
}

void check_write_test(size_t num) {
    printf("checking data...");
    perf_start(PERF_TEST);
    for (uint32_t addr=0; addr<num; addr++) {
        uint8_t expected = addr & 0xFF;
        uint8_t data;
        psram_read(addr, &data, 1);
        if (data != expected) {
            printf("FAIL at address %08x (read %02x vs %02x)\n", addr, data, expected);
            return;
        }
    }
    int64_t tt = perf_end(PERF_TEST);
    float tt_sec = tt / 1E6;
    float mb_per_sec = ((float)num / 1048576) / tt_sec;
    printf("pass (in %.2f sec, %.2f MB/sec)\n", tt_sec, mb_per_sec);
}



int main() {
    hw_init();

    sleep_ms(5000);

    gpio_init(PIN_SPI_MISO);
    gpio_init(PIN_SPI_CS);
    gpio_init(PIN_SPI_SCK);
    gpio_init(PIN_SPI_MOSI);
    gpio_set_dir(PIN_SPI_CS, GPIO_OUT);
    gpio_put(PIN_SPI_CS, 1);
    gpio_set_function(PIN_SPI_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SPI_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SPI_MOSI, GPIO_FUNC_SPI);

    int spi_rate_hz = 25000000;

    spi_init(spi0, spi_rate_hz);

    // Initialise PSRAM
    psram_cmd(0x66);
    sleep_us(50);
    psram_cmd(0x99);
    sleep_us(100);

    uint8_t data[] = {0x55, 0x13, 0xAA, 0x27};

    printf("baud rate is %.2f MHz.\n", spi_rate_hz/1000000.0);
    psram_write(0x1234, data, 4);
    psram_write(0x11234, &data[0], 1);

    sleep_us(1000);

    size_t bytes = 512*1024;
    write_test_1(bytes);
    check_write_test(bytes);
    write_test_4(bytes);
    check_write_test(bytes);

    // uint8_t buffer[4];
    // psram_read(0x1234, buffer, 4);

    // printf("read data:\n");
    // for (int i=0; i<4; i++) {
    //     printf("%02x ", buffer[i]);
    // }
    // printf("\n");

    // uint8_t val;
    // psram_read(0x11235, &val, 1);
    // printf("%02x\n", val);

    printf("done.\n");
    while (1) {
        hw_debug_led(1);
        sleep_ms(500);
        hw_debug_led(0);
        sleep_ms(500);
    }
}
