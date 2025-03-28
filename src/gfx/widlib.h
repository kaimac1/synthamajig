#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stdbool.h>

#define NUM_KNOBS 4
#define SCROLL_KNOB 0
#define DATA_KNOB 1

// Menu item drawing function.
// Takes a name and value string to be displayed
// pos is the relative position of the item on the screen, 0 at the top.
typedef void (ItemDrawFunc)(const char* name, const char* value, int pos, bool selected);

// Call once per frame
// Tell the library by how much the encoders/knobs have moved
void wl_update_knobs(int *delta);


/////
// Menu

// Create a menu. Call this first.
// title is stored and used to identify the menu, to see if we are a drawing a different one
// visible_items is the number of items to show at once
void wl_menu_start(const char *title, int visible_items);
// Then call this to set the draw function for a menu item.
void wl_menu_item_draw_func(ItemDrawFunc *func);
// Then call these functions to create menu items. Returns true if the item is currently selected.
bool wl_menu_item_int(const char *name, int value);
// If an item is selected, call these to edit the associated value. Returns true if the value was changed.
bool wl_menu_edit_int(int *value, int min, int max);
// Finally call this at the end of the menu. 
void wl_menu_end(void);


// Modify value (from min to max) by turning the given knob
// Draw a gauge in the appropriate position
bool kmgui_gauge(int knob, int *value, int min, int max, const char *text);

#ifdef __cplusplus
}
#endif
