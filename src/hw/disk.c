#include "ff.h"
#include "f_util.h"
#include "disk.h"
#include "pinmap.h"
#include "common.h"
#include "nandflash/nandflash.h"
#include "nandflash/nand_spi.h"
#include <stdio.h>
#include <tusb.h>
#include <bsp/board_api.h>

#define DEBUGF printf

#define SAMPLES_DIR "samples"
#define TRACKS_DIR "tracks"

FATFS fs;
struct dhara_map map;
struct dhara_nand nand_parameters;
uint8_t dhara_page_buffer[2048 + 128];
extern bool disk_ejected;

void periodic_task(void);

// Create a directory if it doesn't already exist
int ensure_directory(const char *name) {
    FILINFO info;
    if (f_stat(name, &info) == FR_OK) {
        if (info.fattrib & AM_DIR) {
            return 0;
        } else {
            // File exists with this name, so delete it
            f_unlink(name);
        }
    }

    if (f_mkdir(name) == FR_OK) {
        DEBUGF("Created dir %s\n", name);
        return 0;
    } else {
        return -1;
    }
}

FRESULT create_filesystem(void) {
    const int len = 8192;
    uint8_t buf[len];

    MKFS_PARM opt;
    opt.fmt = FM_FAT;
    opt.n_fat = 1;
    opt.align = 1;
    opt.n_root = 512;
    opt.au_size = DISK_AU_SIZE;

    return f_mkfs("", &opt, buf, len);
}

// Initialise NAND device and flash translation layer
DiskError disk_lowlevel_init(void) {
    dhara_error_t error;

    gpio_init(PIN_NAND_CS);
    gpio_set_dir(PIN_NAND_CS, GPIO_OUT);
    gpio_put(PIN_NAND_CS, 1);

    nand_spi_init();
    int r = nandflash_init(&nand_parameters);
    if (r) return DISK_NAND_FLASH_INIT_FAILED;

    dhara_map_init(&map, &nand_parameters, dhara_page_buffer, DHARA_GC_RATIO);
    r = dhara_map_resume(&map, &error);
    if (r) {
        // Expected for this to return -1 (error 3: too many bad blocks) for a blank device
        INIT_PRINTF("  map_resume returned %d, error %d\n", r, error);
    }

    uint32_t total_sectors = dhara_map_capacity(&map);
    float capacity_mb = total_sectors * DISK_SECTOR_SIZE / (1024.0f * 1024.0f);
    INIT_PRINTF("  capacity = %d (%.2f MB)\n", total_sectors, capacity_mb);
    uint32_t used_sectors = dhara_map_size(&map);
    float used_mb = used_sectors * DISK_SECTOR_SIZE / (1024.0f * 1024.0f);
    INIT_PRINTF("  used = %d (%.3f MB)\n", used_sectors, used_mb);

    return DISK_OK;
}

DiskError disk_init(void) {
    int r = disk_lowlevel_init();
    if (r) return r;

    // Mount
    FRESULT res = f_mount(&fs, "", 1);
    if (res != FR_OK) {
        DEBUGF("f_mount error: %s (%d)\n", FRESULT_str(res), res);
        DEBUGF("Creating filesystem...\n");

        res = create_filesystem();
        if (res) {
            DEBUGF("f_mkfs error: %d\n", res);
            return DISK_FILESYSTEM_ERROR;
        }

        // Try to mount again
        res = f_mount(&fs, "", 1);
        if (res) {
            DEBUGF("failed to mount new filesystem: %d\n", res);
            return DISK_FILESYSTEM_ERROR;
        }
    }
    DEBUGF("Disk mounted\n");

    // Check/create directories
    if (ensure_directory(SAMPLES_DIR)) {
        return DISK_FILESYSTEM_ERROR;
    }

    return 0;
}

void disk_deinit(void) {
    
}

void disk_enter_mass_storage_mode(void) {
    f_unmount("");

    // init device stack on configured roothub port
    tusb_rhport_init_t dev_init = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_AUTO
    };
    tusb_init(BOARD_TUD_RHPORT, &dev_init);

    if (board_init_after_tusb) {
        board_init_after_tusb();
    }

    while (!disk_ejected) {
        tud_task(); // tinyusb device task
        periodic_task();
    }
    // FIXME: deinitialise tusb - somehow
}


//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void) {
    printf("mounted\n");
}

// Invoked when device is unmounted
void tud_umount_cb(void) {
    printf("unmounted\n");
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en) {
    (void) remote_wakeup_en;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void) {
    //blink_interval_ms = tud_mounted() ? BLINK_MOUNTED : BLINK_NOT_MOUNTED;
}

void periodic_task(void) {
    const uint32_t period_ms = 1000;
    static uint32_t start_ms = 0;
    static bool led_state = false;

    if (board_millis() - start_ms < period_ms) return; // not enough time
    start_ms += period_ms;

    // Periodically sync writes
    dhara_error_t error;
    dhara_map_sync(&map, &error);  

    board_led_write(led_state);
    led_state = 1 - led_state; // toggle
}
