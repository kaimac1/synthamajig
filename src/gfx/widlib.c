// widlib
// Immediate-mode widget library

#include "widlib.h"
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
    wlListDrawFuncs *funcs;
    char last_name[TXTLEN];
} Menu;
Menu menu;

int knob_delta[NUM_KNOBS];

void wl_update_knobs(int *delta) {
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


void wl_list_start(const char *title, int visible_items, wlListDrawFuncs *draw_funcs) {
    if (strcmp(menu.title, title)) {
        // Reset
        menu.built = false;
        menu.num_items = 0;
        menu.selected_item = 0;
        menu.top_item = 0;
        menu.num_visible_items = visible_items;
        strlcpy(menu.title, title, sizeof(menu.title));
        menu.funcs = draw_funcs;
    }
    menu.scan_item = -1;

    if (menu.built && knob_delta[SCROLL_KNOB]) {
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

    menu.funcs->draw_menu(menu.title, menu.num_visible_items);
}

bool wl_list_item_int(const char *name, int value) {
    MENU_NEXT();
    if (!MENU_ITEM_VISIBLE()) return false;

    const bool selected = MENU_ITEM_SELECTED();
    char valuebuf[TXTLEN];
    snprintf(valuebuf, sizeof(valuebuf), "%d", value);
    menu.funcs->draw_item(name, valuebuf, MENU_ITEM_POS(), selected);

    // Save name for if we need to redraw
    strlcpy(menu.last_name, name, sizeof(menu.last_name));
    return (menu.built && selected);
}

bool wl_list_item_str(const char *name, const char *value) {
    MENU_NEXT();
    if (!MENU_ITEM_VISIBLE()) return false;

    const bool selected = MENU_ITEM_SELECTED();
    menu.funcs->draw_item(name, value, MENU_ITEM_POS(), selected);
    strlcpy(menu.last_name, name, sizeof(menu.last_name));
    return (menu.built && selected);
}

bool wl_list_edit_int(int *value, int min, int max) {
    // Change value by delta_data
    bool changed = false;
    if (MENU_ITEM_SELECTED() && knob_delta[DATA_KNOB]) {
        int newval = *value + knob_delta[DATA_KNOB];
        if (newval < min) newval = min;
        if (newval > max) newval = max;
        *value = newval;
        changed = true;

        // Redraw item
        char valuebuf[TXTLEN];
        snprintf(valuebuf, sizeof(valuebuf), "%d", newval);
        menu.funcs->draw_item(menu.last_name, valuebuf, MENU_ITEM_POS(), true);
    }

    return changed;
}

void wl_list_end(void) {
    MENU_END();
    menu.funcs->draw_scrollbar(menu.num_items, menu.num_visible_items, menu.top_item);
}




/**************************************************************/
// Gauges

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