#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "dhara/map.h"

// Can't change these parameters without fully erasing the device
#define DHARA_GC_RATIO      8
#define DISK_SECTOR_SIZE    2048
#define DISK_AU_SIZE        16384
#define DISK_NUM_SECTORS    51200   // 51200 is 100 MiB which is just below what dhara gives us

extern struct dhara_map map;

int disk_init(void);
void disk_deinit(void);
void disk_enter_mass_storage_mode(void);

#ifdef __cplusplus
}
#endif