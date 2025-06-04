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

    puts("Initing PSRAM...");
    psram_spi_inst_t psram_spi = psram_spi_init(pio0, -1);

    uint32_t psram_begin, psram_elapsed;
    float psram_speed;

    puts("Testing PSRAM...");

    // **************** 8 bits testing ****************
    psram_begin = time_us_32();
    for (uint32_t addr = 0; addr < (8 * 1024 * 1024); ++addr) {
        psram_write8(&psram_spi, addr, (addr & 0xFF));
    }
    psram_elapsed = time_us_32() - psram_begin;
    psram_speed = 1000000.0 * 8 / psram_elapsed;
    printf("8 bit: PSRAM write 8MB in %d us, %.2f MB/s\n", psram_elapsed, psram_speed);

    /*psram_begin = time_us_32();
    for (uint32_t addr = 0; addr < (8 * 1024 * 1024); ++addr) {
        psram_write8_async(&psram_spi, addr, (addr & 0xFF));
    }
    psram_elapsed = time_us_32() - psram_begin;
    psram_speed = 1000000.0 * 8 * 1024.0 * 1024 / psram_elapsed;
    printf("8 bit: PSRAM write async 8MB in %d us, %d B/s\n", psram_elapsed, psram_speed);*/

    psram_begin = time_us_32();
    for (uint32_t addr = 0; addr < (8 * 1024 * 1024); ++addr) {
        uint8_t result = psram_read8(&psram_spi, addr);
        if ((uint8_t)(addr & 0xFF) != result) {
            printf("\nPSRAM failure at address %x (%x != %x)\n", addr, addr & 0xFF, result);
            return 1;
        }
    }
    psram_elapsed = time_us_32() - psram_begin;
    psram_speed = 1000000.0 * 8 / psram_elapsed;
    printf("8 bit: PSRAM read 8MB in %d us, %.2f MB/s\n", psram_elapsed, psram_speed);

    // **************** 16 bits testing ****************
    psram_begin = time_us_32();
    for (uint32_t addr = 0; addr < (8 * 1024 * 1024); addr += 2) {
        psram_write16(&psram_spi, addr, (((addr + 1) & 0xFF) << 8) | (addr & 0xFF));
    }
    psram_elapsed = time_us_32() - psram_begin;
    psram_speed = 1000000.0 * 8 / psram_elapsed;
    printf("16 bit: PSRAM write 8MB in %d us, %.2f MB/s\n", psram_elapsed, psram_speed);

    psram_begin = time_us_32();
    for (uint32_t addr = 0; addr < (8 * 1024 * 1024); addr += 2) {
        uint16_t result = psram_read16(&psram_spi, addr);
        if ((uint16_t)(
                (((addr + 1) & 0xFF) << 8) |
                (addr & 0xFF)) != result
        ) {
            printf("PSRAM failure at address %x (%x != %x) ", addr, (
                (((addr + 1) & 0xFF) << 8) |
                (addr & 0xFF)), result
            );
            return 1;
        }
    }
    psram_elapsed = (time_us_32() - psram_begin);
    psram_speed = 1000000.0 * 8 / psram_elapsed;
    printf("16 bit: PSRAM read 8MB in %d us, %.2f MB/s\n", psram_elapsed, psram_speed);

    // **************** 32 bits testing ****************
    psram_begin = time_us_32();
    for (uint32_t addr = 0; addr < (8 * 1024 * 1024); addr += 4) {
        psram_write32(
            &psram_spi, addr,
            (uint32_t)(
                (((addr + 3) & 0xFF) << 24) |
                (((addr + 2) & 0xFF) << 16) |
                (((addr + 1) & 0xFF) << 8)  |
                (addr & 0XFF))
        );
    }
    psram_elapsed = time_us_32() - psram_begin;
    psram_speed = 1000000.0 * 8 / psram_elapsed;
    printf("32 bit: PSRAM write 8MB in %d us, %.2f MB/s\n", psram_elapsed, psram_speed);

    psram_begin = time_us_32();
    for (uint32_t addr = 0; addr < (8 * 1024 * 1024); addr += 4) {
        uint32_t result = psram_read32(&psram_spi, addr);
        if ((uint32_t)(
            (((addr + 3) & 0xFF) << 24) |
            (((addr + 2) & 0xFF) << 16) |
            (((addr + 1) & 0xFF) << 8)  |
            (addr & 0XFF)) != result
        ) {
            printf("PSRAM failure at address %x (%x != %x) ", addr, (
                (((addr + 3) & 0xFF) << 24) |
                (((addr + 2) & 0xFF) << 16) |
                (((addr + 1) & 0xFF) << 8)  |
                (addr & 0XFF)), result
            );
            return 1;
        }
    }
    psram_elapsed = (time_us_32() - psram_begin);
    psram_speed = 1000000.0 * 8 / psram_elapsed;
    printf("32 bit: PSRAM read 8MB in %d us, %.2f MB/s\n", psram_elapsed, psram_speed);

    // **************** n bits testing ****************
    uint8_t write_data[256];
    for (size_t i = 0; i < 256; ++i) {
        write_data[i] = i;
    }
    psram_begin = time_us_32();
    for (uint32_t addr = 0; addr < (8 * 1024 * 1024); addr += 256) {
        for (uint32_t step = 0; step < 256; step += 16) {
            psram_write(&psram_spi, addr + step, write_data + step, 16);
        }
    }
    psram_elapsed = time_us_32() - psram_begin;
    psram_speed = 1000000.0 * 8 / psram_elapsed;
    printf("128 bit: PSRAM write 8MB in %d us, %.2f MB/s\n", psram_elapsed, psram_speed);

    psram_begin = time_us_32();
    uint8_t read_data[16];
    for (uint32_t addr = 0; addr < (8 * 1024 * 1024); addr += 256) {
        for (uint32_t step = 0; step < 256; step += 16) {
            psram_read(&psram_spi, addr + step, read_data, 16);
            if (memcmp(read_data, write_data + step, 16) != 0) {
                printf("PSRAM failure at address %x", addr);
                return 1;
            }
        }
    }
    psram_elapsed = time_us_32() - psram_begin;
    psram_speed = 1000000.0 * 8 / psram_elapsed;
    printf("128 bit: PSRAM read 8MB in %d us, %.2f MB/s\n", psram_elapsed, psram_speed);


    printf("done.\n");
    while (1) {
        hw_debug_led(1);
        sleep_ms(500);
        hw_debug_led(0);
        sleep_ms(500);
    }


    gpio_init(PIN_SPI_MISO);
    gpio_init(PIN_SPI_CS);
    gpio_init(PIN_SPI_SCK);
    gpio_init(PIN_SPI_MOSI);
    gpio_set_dir(PIN_SPI_CS, GPIO_OUT);
    gpio_put(PIN_SPI_CS, 1);
    gpio_set_function(PIN_SPI_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SPI_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SPI_MOSI, GPIO_FUNC_SPI);
    gpio_set_drive_strength(PIN_SPI_CS, GPIO_DRIVE_STRENGTH_8MA);
    gpio_set_drive_strength(PIN_SPI_SCK, GPIO_DRIVE_STRENGTH_8MA);
    gpio_set_drive_strength(PIN_SPI_MOSI, GPIO_DRIVE_STRENGTH_8MA);    

    int spi_rate_hz = 50000000;

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
