/* glue.c
Copyright 2021 Carl John Kugler III

Licensed under the Apache License, Version 2.0 (the License); you may not use 
this file except in compliance with the License. You may obtain a copy of the 
License at

   http://www.apache.org/licenses/LICENSE-2.0 
Unless required by applicable law or agreed to in writing, software distributed 
under the License is distributed on an AS IS BASIS, WITHOUT WARRANTIES OR 
CONDITIONS OF ANY KIND, either express or implied. See the License for the 
specific language governing permissions and limitations under the License.
*/
/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/
//
//
#include "diskio.h" /* Declarations of disk functions */

#include "../vendor/dhara/map.h"
#include "pico/stdlib.h"

extern struct dhara_map map;

#define TRACE_PRINTF(fmt, args...)
//#define TRACE_PRINTF printf 

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status(BYTE pdrv /* Physical drive number to identify the drive */
) {
    TRACE_PRINTF(">>> %s(%d)\n", __FUNCTION__, pdrv);
    return 0;
}

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize(
    BYTE pdrv /* Physical drive number to identify the drive */
) {
    TRACE_PRINTF(">>> %s\n", __FUNCTION__);
    return 0;
}


/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read(BYTE pdrv,  /* Physical drive number to identify the drive */
                  BYTE *buff, /* Data buffer to store read data */
                  LBA_t sector, /* Start sector in LBA */
                  UINT count    /* Number of sectors to read */
) {
    TRACE_PRINTF(">>> %s\n", __FUNCTION__);
    dhara_error_t error;
    TRACE_PRINTF("sector %d, cnt %d\n", sector, count);
    // read *count* consecutive sectors
    for (int i = 0; i < count; i++) {
        int ret = dhara_map_read(&map, sector, buff, &error);
        if (ret) {
            // printf("dhara read failed: %d, error: %d", ret, err);
            return RES_ERROR;
        }
        buff += 2048; // sector size == page size
        sector++;
    }    
    return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write(BYTE pdrv, /* Physical drive number to identify the drive */
                   const BYTE *buff, /* Data to be written */
                   LBA_t sector,     /* Start sector in LBA */
                   UINT count        /* Number of sectors to write */
) {
    TRACE_PRINTF(">>> %s\n", __FUNCTION__);
    dhara_error_t error;
    // write *count* consecutive sectors
    for (int i = 0; i < count; i++) {
        int ret = dhara_map_write(&map, sector, buff, &error);
        if (ret) {
            // printf("dhara write failed: %d, error: %d", ret, err);
            return RES_ERROR;
        }
        buff += 2048; // sector size == page size
        sector++;
    }
    return RES_OK;
}

#endif

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl(BYTE pdrv, /* Physical drive number (0..) */
                   BYTE cmd,  /* Control code */
                   void *buff /* Buffer to send/receive control data */
) {
    TRACE_PRINTF(">>> %s(%d)\n", __FUNCTION__, cmd);
    switch (cmd) {
        case GET_SECTOR_COUNT: {  // Retrieves number of available sectors, the
                                  // largest allowable LBA + 1, on the drive
                                  // into the LBA_t variable pointed by buff.
                                  // This command is used by f_mkfs and f_fdisk
                                  // function to determine the size of
                                  // volume/partition to be created. It is
                                  // required when FF_USE_MKFS == 1.
            *(LBA_t *)buff = 16384;
            return RES_OK;
        }
        case GET_BLOCK_SIZE: {  // Retrieves erase block size of the flash
                                // memory media in unit of sector into the DWORD
                                // variable pointed by buff. The allowable value
                                // is 1 to 32768 in power of 2. Return 1 if the
                                // erase block size is unknown or non flash
                                // memory media. This command is used by only
                                // f_mkfs function and it attempts to align data
                                // area on the erase block boundary. It is
                                // required when FF_USE_MKFS == 1.
            static DWORD bs = 64;
            *(DWORD *)buff = bs;
            return RES_OK;
        }
        case GET_SECTOR_SIZE:
            *(WORD*)buff = 2048;
            return RES_OK;       
        case CTRL_SYNC:
            dhara_error_t error;
            dhara_map_sync(&map, &error);
            //sd_card_p->sync(sd_card_p);
            return RES_OK;
        default:
            return RES_PARERR;
    }
}
