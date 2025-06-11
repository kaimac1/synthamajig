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
#include "dhara/map.h"
#include "shell/shell.h"
#include <stdlib.h>
#include "hardware/watchdog.h"
#include "hw/disk.h"

dhara_map map;


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

void write_char(char c) {
    putchar(c);
}

int erase_flash(int argc, char **argv) {
    printf("erasing...");
    for (int b=0; b<1024; b++) {
        row_address_t row;
        row.block = b;
        nandflash_block_erase(row);
    }
    printf("done. reset now\n");
    return 0;
}

int write_sector(int argc, char **argv) {
    if (argc != 3) return -1;
    uint32_t sector = atoi(argv[1]);
    uint8_t sectordata[2048];
    memset(sectordata, 0, sizeof(sectordata));
    strcpy((char*)sectordata, argv[2]);

    dhara_error_t error;
    int r = dhara_map_write(&map, sector, sectordata, &error);
    if (r) {
        INIT_PRINTF("  write returned %d, error %d\n", r, error);
    }    
    return 0;
}

int read_sector(int argc, char **argv) {
    if (argc != 2) return -1;
    uint32_t sector = atoi(argv[1]);
    uint8_t sectordata[2048];
    memset(sectordata, 0, sizeof(sectordata));

    dhara_error_t error;
    int r = dhara_map_read(&map, sector, sectordata, &error);
    if (r) {
        INIT_PRINTF("  read returned %d, error %d\n", r, error);
    } else {
        printf("Sector data: %s\n", sectordata);
    }    

    return 0;
}

int disk_sync(int argc, char **argv) {
    dhara_error_t error;
    int r = dhara_map_sync(&map, &error);
    if (r) {
        INIT_PRINTF("  sync returned %d, error %d\n", r, error);
    }    
    return 0;
}

int disk_gc(int argc, char**argv) {
    dhara_error_t error;
    int num = 1;
    if (argc == 2) {
        num = atoi(argv[1]);
    }
    for (int i; i<num; i++) {
        int r = dhara_map_gc(&map, &error);
        if (r) {
            INIT_PRINTF("  gc returned %d, error %d\n", r, error);
        }    
    }
    return 0;
}

int reset_mcu(int argc, char **argv) {
    watchdog_reboot(0, 0, 100);
    return 0;
}

int read_test(int argc, char **argv) {
    uint8_t sectordata[2048];
    dhara_error_t error;
    const size_t sectors = 256;
    const size_t total = sectors * 2048;
    uint32_t time_begin = time_us_32();
    for (int i=0; i<sectors; i+=8) {
        dhara_map_read(&map, 0, sectordata, &error);
        dhara_map_read(&map, 1, sectordata, &error);
        dhara_map_read(&map, 2, sectordata, &error);
        dhara_map_read(&map, 3, sectordata, &error);
        dhara_map_read(&map, 4, sectordata, &error);
        dhara_map_read(&map, 5, sectordata, &error);
        dhara_map_read(&map, 6, sectordata, &error);
        dhara_map_read(&map, 7, sectordata, &error);        
    }
    uint32_t time_elapsed = time_us_32() - time_begin;
    float speed_bps = 1000000.0f * total / time_elapsed;
    printf("%.0f KB/sec\n", speed_bps/1024);

    return 0;
}

int file_test(int argc, char **argv) {
    FIL file;
    int r = f_open(&file, "test", FA_WRITE|FA_OPEN_ALWAYS);
    printf("%d\n", r);
    unsigned int bw;
    uint8_t buffer[8192];
    uint32_t time_begin = time_us_32();
    for (int i=0; i<32; i++) {
        r = f_write(&file, buffer, sizeof(buffer), &bw);
        if (r) printf("wr %d\n", r);
    }
    f_close(&file);
    const size_t total = 8192*32;
    uint32_t time_elapsed = time_us_32() - time_begin;
    float speed_bps = 1000000.0f * total / time_elapsed;
    printf("%.0f KB/sec\n", speed_bps/1024);    
    return 0;
}

int file_read_test(int argc, char **argv) {
    FIL file;
    int r = f_open(&file, "test", FA_READ);
    printf("%d\n", r);
    unsigned int bw;
    uint8_t buffer[8192];
    uint32_t time_begin = time_us_32();
    for (int i=0; i<32; i++) {
        r = f_read(&file, buffer, sizeof(buffer), &bw);
        if (r) printf("rd %d\n", r);
    }
    f_close(&file);
    const size_t total = 8192*32;
    uint32_t time_elapsed = time_us_32() - time_begin;
    float speed_bps = 1000000.0f * total / time_elapsed;
    printf("%.0f KB/sec\n", speed_bps/1024);    
    return 0;
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

    // For shell
    set_read_char(getchar);
    set_write_char(write_char);

    sleep_ms(2000);

    INIT_PRINTF("Initialising PSRAM...\n");
    psram_spi_inst_t psram = psram_spi_init(pio0);
    if (psram.error) {
        while (1);
    }

    ADD_CMD("erase", "erase disk", erase_flash);
    ADD_CMD("ws", "write sector", write_sector);
    ADD_CMD("rs", "read sector", read_sector);
    ADD_CMD("sync", "sync disk", disk_sync);
    ADD_CMD("gc", "gc disk", disk_gc);
    ADD_CMD("reset", "reset MCU", reset_mcu);
    ADD_CMD("readtest", "read test", read_test);
    ADD_CMD("filetest", "file write test", file_test);
    ADD_CMD("frd", "file read test", file_read_test);


    dhara_nand nand_parameters;

    nand_spi_init();
    int r = nandflash_init(&nand_parameters);
    INIT_PRINTF("  init returned %d\n", r);

    const size_t sector_size = 1 << nand_parameters.log2_page_size;

    dhara_error_t error;
    const uint8_t gc_ratio = 8;
    uint8_t page_buffer[2300];
    dhara_map_init(&map, &nand_parameters, page_buffer, gc_ratio);

    r = dhara_map_resume(&map, &error);
    if (r) {
        // Expected for this to return -1 (error 3: too many bad blocks) for a blank device
        INIT_PRINTF("  map_resume returned %d, error %d\n", r, error);
    }

    uint32_t total_sectors = dhara_map_capacity(&map);
    float capacity_mb = total_sectors * sector_size / (1024.0f * 1024.0f);
    INIT_PRINTF("  capacity = %d (%.2f MB)\n", total_sectors, capacity_mb);

    uint32_t used_sectors = dhara_map_size(&map);
    float used_mb = used_sectors * sector_size / (1024.0f * 1024.0f);
    INIT_PRINTF("  used = %d (%.3f MB)\n", used_sectors, used_mb);

    disk_init();

    prompt();

    // uint8_t buffer[256];
    // row_address_t row;
    // row.whole = 0;
    // r = nandflash_page_read(row, 0, buffer, sizeof(buffer));
    // INIT_PRINTF("read=%d\n",r);
    // INIT_PRINTF("buffer= %02x %02x %02x %02x\n", buffer[0], buffer[1], buffer[2], buffer[3]);    

    // // uint8_t wbuffer[4] = {1, 2, 3, 4};
    // // r = nandflash_page_program(row, 0, wbuffer, sizeof(wbuffer));
    // // INIT_PRINTF("write=%d\n",r);    

    // for (uint32_t block=0; block<1024; block++) {
    //     for (uint32_t page=0; page<64; page++) {
    //         row_address_t row = {.page = page, .block = block};
    //         bool free;
    //         r = nandflash_page_is_free(row, &free);
    //         if (r) {
    //             INIT_PRINTF("error %d\n", r);
    //             while (1);
    //         }
    //         INIT_PRINTF("%c", free ? '.' : 'x');
    //     }
    //     if (block % 4 == 3) INIT_PRINTF("\n");
    // }


    // row_address_t row;
    // row.whole = 0;
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
