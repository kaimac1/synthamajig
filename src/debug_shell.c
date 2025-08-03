#include "debug_shell.h"
#include "common.h"
#include "nandflash/nandflash.h"
#include <stdlib.h>
#include "hardware/watchdog.h"
#include "ff.h"
#include "hw/disk.h"
#include "hw/psram_spi.h"
#include "hw/hw.h"
#include "hw/pinmap.h"

void write_char(char c) {
    putchar(c);
}

int enter_msc(int argc, char **argv) {
    //disk_enter_mass_storage_mode();
    return 0;
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

int ram_test(int argc, char **argv) {
    extern psram_spi_inst_t psram;
    psram_test(&psram);
    return 0;

}

int led_test(int argc, char **argv) {

    for (int n=0; n<8; n++) {
        for (int i=0; i<255; i++) {
            uint8_t val = (i*i) >> 8;
            for (int k=0; k<5; k++) {
                hw_set_led(k*3, val);
            }
            sleep_ms(1);
        }

        for (int i=255; i>=0; i--) {
            uint8_t val = (i*i) >> 8;
            for (int k=0; k<5; k++) {
                hw_set_led(k*3, val);
            }
            sleep_ms(1);
        }
    }

    return 0;
}


void debug_shell_init(void) {
    set_read_char(getchar);
    set_write_char(write_char);

    ADD_CMD("erase", "erase disk", erase_flash);
    ADD_CMD("ws", "write sector", write_sector);
    ADD_CMD("rs", "read sector", read_sector);
    ADD_CMD("sync", "sync disk", disk_sync);
    ADD_CMD("gc", "gc disk", disk_gc);
    ADD_CMD("reset", "reset MCU", reset_mcu);
    ADD_CMD("readtest", "read test", read_test);
    ADD_CMD("filetest", "file write test", file_test);
    ADD_CMD("frd", "file read test", file_read_test);
    ADD_CMD("ramtest", "PSRAM test", ram_test);
    ADD_CMD("msc", "mass storage mode", enter_msc);
    ADD_CMD("ledtest", "led test", led_test);
}