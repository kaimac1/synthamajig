#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stdbool.h>

#define NUM_KNOBS 4
#define SCROLL_KNOB 0
#define DATA_KNOB 1


typedef void (DrawMenuFunc)(const char* title, int num_items);

// Menu item drawing function.
// Takes a name and value string to be displayed
// pos is the relative position of the item on the screen, 0 at the top.
typedef void (DrawItemFunc)(const char* name, const char* value, int pos, bool selected);

typedef void (DrawScrollbarFunc)(int total_items, int visible_items, int top_item);

typedef struct {
    DrawMenuFunc *draw_menu;
    DrawItemFunc *draw_item;
    DrawScrollbarFunc *draw_scrollbar;
} wlListDrawFuncs;


// Call once per frame
// Tell the library by how much the encoders/knobs have moved
void wl_update_knobs(int *delta);


/////
// List

// Create a list. Call this first.
// title is stored and used to identify the list, to see if we are a drawing a different one
// visible_items is the number of items to show at once
void wl_list_start(const char *title, int visible_items, wlListDrawFuncs *draw_funcs);

// Then call these functions to create list items. Returns true if the item is currently selected.
bool wl_list_item_int(const char *name, int value);
bool wl_list_item_str(const char *name, const char *value);

// If an item is selected, call these to edit the associated value. Returns true if the value was changed.
bool wl_list_edit_int(int *value, int min, int max);

// Finally call this at the end of the list. 
void wl_list_end(void);


// Modify value (from min to max) by turning the given knob
// Draw a gauge in the appropriate position
bool kmgui_gauge(int knob, int *value, int min, int max, const char *text);

#ifdef __cplusplus
}
#endif
