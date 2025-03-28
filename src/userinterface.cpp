#include "userinterface.hpp"
#include "track.hpp"
#include "common.h"
#include "hw/oled.h"
#include "gfx/ngl.h"
#include "gfx/kmgui.h"
#include "assets/assets.h"
#include <cstdio>
#include <string.h>

#define CHANNEL (track.channels[track.active_channel])
#define PATTERN (CHANNEL.pattern)

// Contains channels, instruments, parameters, pattern data
Track track;

bool keys_pressed;
int pattern_page;           // Which page of the pattern we can currently see




static void set_brightness(int level) {
    oled_set_brightness(25 * level);
}

static void set_channel(int ch) {
    track.active_channel = ch;
}

static int next_pattern_page(void) {
    int num_pages = (PATTERN.length + NUM_STEPKEYS-1) / NUM_STEPKEYS; // round up
    return (pattern_page + 1) % num_pages;
}

// Get the step in the pattern corresponding to the given key.
// May return null, as the key could be beyond the end of the pattern.
static Step *get_step_from_key(int key) {
    int stepidx = pattern_page*NUM_STEPKEYS + key;
    if (stepidx >= PATTERN.length) return NULL;
    return &PATTERN.step[stepidx];
}




void UI::init() {
    set_brightness(brightness);

    view = VIEW_TRACK;

    track.reset();
    track.set_volume_percent(volume_percent);
    track.bpm = 120;

    // demo pattern
    #define PATTERN_LEN 16
    const int notes[PATTERN_LEN] = {42,40,42,42,40,42,47,42,42,49,46,49,40,45,42,47};
    const int    on[PATTERN_LEN] = { 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1};
    const int accts[PATTERN_LEN] = { 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0};
    Pattern *p = &track.channels[0].pattern;
    for (int i=0; i<PATTERN_LEN; i++) {
        p->step[i].on = on[i];
        p->step[i].gate_length = 96;
        if (on[i]) {
            p->step[i].note.freq = note_table[notes[i]];
            p->step[i].note.trigger = 1;
            p->step[i].note.accent  = accts[i];
        }        
    }
    p->length = PATTERN_LEN;

    // drum
    p = &track.channels[2].pattern;
    p->length = 4;
    p->step[0].on = true;
    p->step[0].sample_id = 0;
}


// Button macros for readability
#define PRESSED(btn) btn_press(&inputs, (btn))
#define RELEASED(btn) btn_release(&inputs, (btn))
#define MOD_PRESSED(mod, btn) (btn_down(&inputs, (mod)) && btn_press(&inputs, (btn)))

bool UI::process(RawInput in) {

    bool update = false;

    track.schedule();   // Run sequencer

    if (input_process(&inputs, in)) {
        update = true;

        for (int i=0; i<NUM_STEPKEYS; i++) {
            if (PRESSED(i)) {
                keys_pressed = true;
                break;
            }
        }

        // Instrument pages
        if (MOD_PRESSED(BTN_CHAN, MODBTN_FILTER)) {
            view = VIEW_INSTRUMENT;
            track.instrument_page = INSTRUMENT_PAGE_FILTER;
        } else if (MOD_PRESSED(BTN_CHAN, MODBTN_AMP)) {
            view = VIEW_INSTRUMENT;
            track.instrument_page = INSTRUMENT_PAGE_AMP;


        } else if (PRESSED(BTN_TRACK)) {
            view = VIEW_TRACK;
        } else if (PRESSED(BTN_MENU)) {
            view = VIEW_DEBUG_MENU;
        } else if (PRESSED(BTN_CHAN)) {
            view = VIEW_CHANNELS;
        } else if (PRESSED(BTN_PATTERN)) {
            if (view != VIEW_PATTERN) {
                view = VIEW_PATTERN;
                pattern_page = 0;
            } else {
                pattern_page = next_pattern_page();
            }
            
        }

        // Play/stop
        if (PRESSED(BTN_PLAY)) {
            track.play(!track.is_playing);
        }

        // Record
        if (PRESSED(BTN_REC)) {
            recording = !recording;
        }

        // Keyboard
        if (PRESSED(BTN_KEYBOARD)) {
            track.enable_keyboard(!track.keyboard_enabled);
            keys_pressed = false;
        } else if (RELEASED(BTN_KEYBOARD)) {
            // Quick mode
            if (track.keyboard_enabled && keys_pressed) {
                track.enable_keyboard(false);
            }
        }
    }

    // For gui widgets
    kmgui_update_knobs(inputs.knob_delta);

    if (track.is_playing) update = true;

    // Update could also be set on timer expiry here

    if (update) {
        perf_start(PERF_DRAWTIME);
        ngl_fillscreen(0);

        switch (view) {
        case VIEW_INSTRUMENT:
            track.channels[track.active_channel].inst->draw(track.instrument_page);
            break;

        case VIEW_DEBUG_MENU:
            debug_menu();
            break;

        case VIEW_TRACK:
            track_page();
            break;
        
        case VIEW_PATTERN:
            view_pattern();
            break;

        case VIEW_STEP_EDIT:
            view_step();
            break;

        case VIEW_CHANNELS:
            view_all_channels();
            break;

        case VIEW_CHANNEL_EDIT:
            view_channel();
            break;
        }

        //switch (ui.msg) { }

        draw_debug_info();
        perf_end(PERF_DRAWTIME);
    }

    // Set step LEDs
    //if (ui.msg) led_mode = LEDS_OVERRIDDEN;
    if (led_mode != LEDS_OVERRIDDEN) {
        for (int i=0; i<NUM_STEPKEYS; i++) {
            uint8_t led = LED_OFF;
            
            if (led_mode == LEDS_SHOW_CHANNELS) {
                // Show activity on each channel
                if (i < NUM_CHANNELS) {
                    if (track.get_channel_activity(i)) {
                        led = LED_ON;
                    } else {
                        led = LED_DIM;
                    }
                }
            } else if (led_mode == LEDS_SHOW_STEPS) {
                int pidx = pattern_page*NUM_STEPKEYS + i;
                if (PATTERN.step[pidx].on && pidx < PATTERN.length) led = LED_DIM;
                if (track.is_playing) {
                    if (CHANNEL.step == pidx) led = LED_ON;
                }
            }
            hw_set_led(i, led);
        }
    }

    return update;


}




// The draw function for a debug menu item
void debug_menu_item(const char *name, const char *value, int pos, bool selected) {
    const int flags = selected ? TEXT_INVERT : 0;
    const int ypos = 16 * pos;
    if (selected) ngl_rect(0, 16+ypos, 128,16, 1);
    draw_text(2, 18+ypos, flags, name);
    if (value) {
        draw_text(127, 18+ypos, flags | TEXT_ALIGN_RIGHT, value);
    }
}

void UI::debug_menu() {

    kmgui_menu_start("Debug menu", 5);
    kmgui_menu_item_draw_func(debug_menu_item);
    if (kmgui_menu_item_int("Volume", volume_percent)) {
        if (kmgui_menu_edit_int(&volume_percent, 0, 100)) {
            track.set_volume_percent(volume_percent);    
        }
    }
    if (kmgui_menu_item_int("Brightness", brightness)) {
        if (kmgui_menu_edit_int(&brightness, 0, 10)) {
            set_brightness(brightness);
        }
    }
    kmgui_menu_item_int("Test", 1);
    kmgui_menu_item_int("Testing", 2);
    kmgui_menu_item_int("Tested", 3);
    kmgui_menu_item_int("Testable", 4);
    kmgui_menu_item_int("Testest", 5);
    kmgui_menu_end();    

    // soft keys
    // draw_text(64,50,TEXT_CENTRE, "Delete?");
    // draw_text(120,82,TEXT_ALIGN_RIGHT, "Yes");
    // draw_text(120,102,TEXT_ALIGN_RIGHT, "No");
    // draw_line(125,60,127,60,1);
    // draw_line(125,60,125,88,1);
    // draw_line(125,88,123,88,1);
    // draw_line(123,108,127,108,1);

    // worth exploring this as an idea - scrolling pages
    // offs += delta_scroll*4;
    // if (offs >= 5*128) offs = 5*128;
    // if (offs < 0) offs = 0;
    // for (int i=0;i<5; i++) {
    //     draw_bitmap_xclip(i*119-offs,0,testimg,119,128);
    // }

    //draw_bitmap(0,0,testimg,offs,128-offs,128);
    //draw_bitmap(119-offs,0,testimg,0,MIN(offs,120),128);    
}

void UI::track_page() {
    led_mode = LEDS_SHOW_CHANNELS;
    kmgui_gauge(0, &track.bpm, 5, 240, "$ bpm");
}

void UI::view_pattern() {
    led_mode = LEDS_SHOW_STEPS;

    if (track.keyboard_enabled) {
        // Play notes. Enter them into the pattern if in write mode

    } else {
        // Select or toggle steps
        bool shift = btn_down(&inputs, BTN_SHIFT);
        for (int i=0; i<NUM_STEPKEYS; i++) {
            if (btn_press(&inputs, i)) {
                if (recording) {
                    Step *step = get_step_from_key(i);
                    if (step) {
                        step->on ^= 1;
                        if (!shift && step->on) {
                            step->note.freq = track.last_played_freq;
                            step->note.trigger = 1;
                            step->note.accent = 0;
                        }
                    }
                } else {
                    Step *step = get_step_from_key(i);
                    if (step) {
                        selected_step = step;
                        view = VIEW_STEP_EDIT;
                    }
                }
            }
        }
    }

    // Pattern length 
    kmgui_gauge(0, &PATTERN.length, 1, 64, "Len=$");

    // for (int i=0; i<16; i++) {
    //     char buf[2];
    //     buf[0] = '?';//track.pattern.step[i].note.accent ? 'X' : '.';
    //     buf[1] = 0;
    //     int x = 64 - 4*xsp+xsp/2 + (i%8)*xsp;
    //     int y = 32 + ysp*(i/8);
    //     int opt = TEXT_CENTRE;
    //     // if (i == track.stepidx[0]) {
    //     //     opt |= TEXT_INVERT;
    //     //     draw_rect(x-8,y-2,16,16,1);
    //     // }
    //     draw_text(x,y,opt,buf);
    // }
}

void UI::view_step() {
    led_mode = LEDS_SHOW_STEPS;
    draw_text(32,32,0, "Step edit");
    if (selected_step == NULL) return;
    draw_textf(16,48,0, "Freq: %d", selected_step->note.freq);
    draw_textf(16,64,0, "Trig: %d", selected_step->note.trigger);
    draw_textf(16,80,0, "Samp: %d", selected_step->sample_id);
}


void UI::channel_modes_common() {
    led_mode = LEDS_SHOW_CHANNELS;

    if (!track.keyboard_enabled) {
        for (int i=0; i<NUM_CHANNELS; i++) {
            if (MOD_PRESSED(BTN_SHIFT, i)) {
                track.channels[i].mute(!track.channels[i].is_muted);
            } else if (PRESSED(i)) {
                set_channel(i);
                view = VIEW_CHANNEL_EDIT;
            }
        }
    }
}

// Channel overview
// Show channel activity on LEDs
void UI::view_all_channels() {
    channel_modes_common();
}

void UI::view_channel() {
    channel_modes_common();
    draw_text(32,32,0, "Chan edit");
    draw_textf(16,80,0, "Samp: %d", track.channels[track.active_channel].cur_sample_id);

}

void UI::draw_debug_info(void) {

    char buf[32] = {0};
    if (recording) strcat(buf, "Rec ");
    if (track.keyboard_enabled) strcat(buf, "Kb ");

    draw_textf(0,0,0, "%sCh%d %s", buf, track.active_channel+1, track.is_over_limit ? "[!]" : "");
    //draw_textf(70,0,0, "P%02d", track.pattern_idx+1);
    draw_textf(127,0,TEXT_ALIGN_RIGHT, "%d/%d",
        track.channels[track.active_channel].step+1, track.channels[track.active_channel].pattern.length);
    
    // audio CPU usage
    int64_t time_audio_us = perf_get(PERF_AUDIO);
    float audio_percent = 100.0f * time_audio_us/5805.0f;
    draw_textf(0,115,0,"%.1f%%",  audio_percent);

    //float fps_display = 1E6 / perf_get(PERF_DISPLAY_UPDATE);
    //draw_textf(0,0,0,"%.1f fps", fps_display);
    draw_textf(127,115,TEXT_ALIGN_RIGHT,"draw %d", perf_get(PERF_DRAWTIME));

    //ngl_bitmap(16,16, gauge_test);

    #ifdef DEBUG_AMPLITUDE
    draw_textf(64,64,TEXT_CENTRE,"(%d, %d)", samplemin, samplemax);
    samplemin = 0; samplemax = 0;
    #endif
}


