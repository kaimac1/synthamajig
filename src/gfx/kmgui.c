#include "kmgui.h"
#include "ngl.h"
#include "gfx_ext.h"
#include <string.h>
#include <stdio.h>

#define TXTLEN 32

typedef struct {
    char title[TXTLEN];
    bool built;
    int num_items;
    int selected_item;
    int scan_item;
} Menu;
Menu menu;

int knob_delta[NUM_KNOBS];

void kmgui_update_knobs(int *delta) {
    for (int i=0; i<NUM_KNOBS; i++) {
        knob_delta[i] = delta[i];
    }
}


/**************************************************************/
// Menu

void kmgui_menu_start(const char *title) {
    if (strcmp(menu.title, title)) {
        // reset
        menu.built = 0;
        menu.num_items = 0;
        menu.selected_item = 0;
        strcpy(menu.title, title);
    }
    menu.scan_item = 0;

    draw_text(0,0,0, title);

    if (knob_delta[SCROLL_KNOB]) {
        menu.selected_item += knob_delta[SCROLL_KNOB];
        if (menu.selected_item < 0) menu.selected_item = 0;
        if (menu.selected_item >= menu.num_items) menu.selected_item = menu.num_items-1;
    }
}

bool kmgui_menu_item_int(int *value, const char *name, int min, int max) {
    bool selected = menu.selected_item == menu.scan_item;

    // Change value by delta_data
    if (selected && knob_delta[DATA_KNOB]) {
        int newval = *value + knob_delta[DATA_KNOB];
        if (newval < min) newval = min;
        if (newval > max) newval = max;
        *value = newval;
    }
    
    int flags = selected ? TEXT_INVERT : 0;
    if (selected) ngl_rect(0, 16+16*menu.scan_item, 128,16, 1);
    draw_text(2,    18+16*menu.scan_item, flags, name);
    draw_textf(127, 18+16*menu.scan_item, flags | TEXT_ALIGN_RIGHT, "%d", *value);

    menu.scan_item++;
    if (!menu.built) menu.num_items++;
    return (menu.built && selected && knob_delta[DATA_KNOB]);
}

void kmgui_menu_item_int_readonly(int value, const char *name) {
    bool selected = menu.selected_item == menu.scan_item;

    int flags = selected ? TEXT_INVERT : 0;
    if (selected) ngl_rect(0, 16+16*menu.scan_item, 128,16, 1);
    draw_text(2,    18+16*menu.scan_item, flags, name);
    draw_textf(127, 18+16*menu.scan_item, flags | TEXT_ALIGN_RIGHT, "%d", value);

    menu.scan_item++;
    if (!menu.built) menu.num_items++;
}

void kmgui_menu_end(void) {
    menu.built = true;
}




/**************************************************************/
// Gauges

//bool update_int_from_knob()

// Replace $ in text with value
void format_text_int(char *out, int size, const char *in, int value) {
    int idx = 0;
    for (const char *t = in; *t && idx < size-1; t++) {
        if (*t == '$') {
            idx += snprintf(&out[idx], size-idx, "%d", value);
        } else {
            out[idx++] = *t;
        }
    }
    if (idx >= size) idx = size-1;
    out[idx] = 0;
}



bool kmgui_gauge(int knob, int *value, int min, int max, const char *text) {

    bool changed = knob_delta[knob];
    int newval = *value + knob_delta[knob];
    if (newval < min) newval = min;
    if (newval > max) newval = max;
    *value = newval;

    char buf[TXTLEN];
    format_text_int(buf, sizeof(buf), text, *value);
    
    draw_gauge_custom(knob, *value, min, max, buf);

    return changed;
}