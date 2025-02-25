#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stdbool.h>

#define NUM_KNOBS 4
#define SCROLL_KNOB 0
#define DATA_KNOB 1

// Call once per frame
// Tell kmgui by how much the encoders/knobs have moved
void kmgui_update_knobs(int *delta);

void kmgui_menu_start(const char *title);
bool kmgui_menu_item_int(int *value, const char *name, int min, int max);
void kmgui_menu_item_int_readonly(int value, const char *name);
void kmgui_menu_end(void);


// Modify value (from min to max) by turning the given knob
// Draw a gauge in the appropriate position
bool kmgui_gauge(int knob, int *value, int min, int max, const char *text);

#ifdef __cplusplus
}
#endif
