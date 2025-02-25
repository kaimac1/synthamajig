#pragma once
#ifdef __cplusplus
extern "C" {
#endif

void oled_init(uint8_t *framebuffer);

// Blank/unblank the display
void oled_on(bool on);

void oled_set_brightness(int brightness);

// Make a copy of the framebuffer and start a write to the device (non-blocking)
// Return false if a write is already in progress
bool oled_write(void);

// Return true if a write is in progress
bool oled_busy(void);

#ifdef __cplusplus
}
#endif
