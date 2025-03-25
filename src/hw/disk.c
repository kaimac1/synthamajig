#include "ff.h"
#include "f_util.h"
#include <stdio.h>

FATFS fs;

int disk_init(void) {
    int status = 0;

    FRESULT res = f_mount(&fs, "", 1);
    if (res != FR_OK) {
        status = -1;
        printf("f_mount error: %s (%d)\n", FRESULT_str(res), res);
    } else {
        printf("SD card mounted\n");
    }

    return status;
}



