#include "ff.h"
#include "f_util.h"
#include <stdio.h>

#define DEBUGF printf

#define SAMPLES_DIR "samples"
#define TRACKS_DIR "tracks"

FATFS fs;



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
        DEBUGF("created %s", name);
        return 0;
    } else {
        return -1;
    }
}




int disk_init(void) {

    // Mount
    FRESULT res = f_mount(&fs, "", 1);
    if (res != FR_OK) {
        printf("f_mount error: %s (%d)\n", FRESULT_str(res), res);
        return -1;
    } else {
        printf("SD card mounted\n");
    }

    // Check/create directories
    if (ensure_directory(SAMPLES_DIR)) {
        DEBUGF("failed\n");
        return -1;
    }

    return 0;
}



