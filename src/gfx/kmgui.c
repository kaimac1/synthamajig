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
    int top_item;
    int num_visible_items;
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

#define MENU_ITEM_SELECTED()    (menu.selected_item == menu.scan_item)
#define MENU_ITEM_VISIBLE()     ((menu.scan_item >= menu.top_item) && (menu.scan_item < menu.top_item + menu.num_visible_items))
#define MENU_ITEM_POS()         (menu.scan_item - menu.top_item)
#define MENU_NEXT()             menu.scan_item++; if (!menu.built) menu.num_items++;
#define MENU_END()              menu.built = true;

static void menu_start(const char *title, int num_visible_items) {
    if (strcmp(menu.title, title)) {
        // Reset
        menu.built = false;
        menu.num_items = 0;
        menu.selected_item = 0;
        menu.top_item = 0;
        menu.num_visible_items = num_visible_items;
        strlcpy(menu.title, title, sizeof(menu.title));
    }
    menu.scan_item = 0;

    if (knob_delta[SCROLL_KNOB]) {
        // Change selected item
        menu.selected_item += knob_delta[SCROLL_KNOB];
        if (menu.selected_item < 0) menu.selected_item = 0;
        if (menu.selected_item >= menu.num_items) menu.selected_item = menu.num_items - 1;
        // Scroll window
        if (menu.selected_item < menu.top_item) menu.top_item = menu.selected_item;
        if (menu.selected_item >= menu.top_item + menu.num_visible_items) {
            menu.top_item = menu.selected_item - menu.num_visible_items + 1;
        }
    }    
}

void kmgui_menu_start(const char *title) {
    menu_start(title, 5);
    draw_text(0,0,0, title);
}

bool kmgui_menu_item_int(int *value, const char *name, int min, int max) {
    if (!MENU_ITEM_VISIBLE()) {
        MENU_NEXT();
        return false;
    }

    const bool selected = MENU_ITEM_SELECTED();

    // Change value by delta_data
    if (selected && knob_delta[DATA_KNOB]) {
        int newval = *value + knob_delta[DATA_KNOB];
        if (newval < min) newval = min;
        if (newval > max) newval = max;
        *value = newval;
    }
    
    const int flags = selected ? TEXT_INVERT : 0;
    const int ypos = 16*MENU_ITEM_POS();

    if (selected) ngl_rect(0, 16+ypos, 128,16, 1);
    draw_text(2,    18+ypos, flags, name);
    draw_textf(127, 18+ypos, flags | TEXT_ALIGN_RIGHT, "%d", *value);

    MENU_NEXT();
    return (menu.built && selected && knob_delta[DATA_KNOB]);
}

void kmgui_menu_item_int_readonly(int value, const char *name) {
    if (!MENU_ITEM_VISIBLE()) {
        MENU_NEXT();
        return;
    }

    const bool selected = MENU_ITEM_SELECTED();
    const int flags = selected ? TEXT_INVERT : 0;
    const int ypos = 16*MENU_ITEM_POS();

    if (selected) ngl_rect(0, 16+ypos, 128,16, 1);
    draw_text(2,    18+ypos, flags, name);
    draw_textf(127, 18+ypos, flags | TEXT_ALIGN_RIGHT, "%d", value);

    MENU_NEXT();
}

void kmgui_menu_end(void) {
    MENU_END();
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