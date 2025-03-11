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

UIState ui;
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
    const int notes[PATTERN_LEN] = {32,32,37,32,35,39,35,32,42,39,46,47,32,49,39,49};
    const int    on[PATTERN_LEN] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
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
        if (MOD_PRESSED(BTN_CHAN, SBTN_FILTER)) {
            view = VIEW_INSTRUMENT;
            track.instrument_page = INSTRUMENT_PAGE_FILTER;
        } else if (MOD_PRESSED(BTN_CHAN, SBTN_AMP)) {
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
            pattern_view();
            break;

        case VIEW_CHANNELS:
            channel_overview();
            break;

        case VIEW_CHANNEL_MENU:
            channel_menu();
            break;
        }

        switch (ui.msg) {
        // case MSG_SELECT_VOICE:
        //     select_channel();
        //     break;
        }

        draw_debug_info();
        perf_end(PERF_DRAWTIME);
    }

    // Set step LEDs
    //if (ui.msg) led_mode = LEDS_OVERRIDDEN;
    if (led_mode != LEDS_OVERRIDDEN) {
        for (int i=0; i<NUM_STEPKEYS; i++) {
            uint8_t led = LED_OFF;
            
            if (led_mode == LEDS_SHOW_CHANNELS) {
                // Show gates of each channel
                if (i < NUM_CHANNELS) {
                    if (track.get_channel_gate(i)) {
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








void UI::debug_menu() {

    kmgui_menu_start("Debug menu");
    if (kmgui_menu_item_int(&volume_percent, "Volume", 0, 100)) {
        track.set_volume_percent(volume_percent);
    }
    if (kmgui_menu_item_int(&brightness, "Brightness", 0, 10)) {
        set_brightness(brightness);
    }
    kmgui_menu_item_int_readonly(sizeof(Track), "sz.track");
    kmgui_menu_item_int_readonly(sizeof(Instrument), "sz.inst");
    kmgui_menu_item_int_readonly(sizeof(AcidBass), "sz.acidbass");
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

void UI::pattern_view() {
    const int xsp = 14, ysp = 16;

    led_mode = LEDS_SHOW_STEPS;

    if (track.keyboard_enabled) {
        // Play notes. Enter them into the pattern if in write mode

    } else {
        // Select or toggle steps

        bool shift = btn_down(&inputs, BTN_SHIFT);
        for (int i=0; i<NUM_STEPKEYS; i++) {
            if (recording) {
                if (btn_press(&inputs, i)) {
                    Step *step = get_step_from_key(i);
                    if (step) {
                        step->on ^= 1;
                        if (!shift && step->on) {
                            step->note.freq = track.last_played_freq;
                            step->note.trigger = 1;
                            step->note.accent = 0;
                        }
                    }
                }
            }
        }
    }

    // Pattern length 
    kmgui_gauge(0, &PATTERN.length, 1, 64, "Length=$");

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


void UI::handle_channel_modes() {
    led_mode = LEDS_SHOW_CHANNELS;
    for (int i=0; i<NUM_STEPKEYS; i++) {
        if (btn_press(&inputs, i) && i<NUM_CHANNELS) {
            set_channel(i);
            view = VIEW_CHANNEL_MENU;
        }
    }    
}

// Channel overview
// Show channel activity on LEDs
void UI::channel_overview() {
    handle_channel_modes();
}

void UI::channel_menu() {
    handle_channel_modes();
    draw_text(32,32,0, "Chan menu");

}

void UI::draw_debug_info(void) {

    char buf[32] = {0};
    if (recording) strcat(buf, "Rec ");
    if (track.keyboard_enabled) strcat(buf, "Kb ");

    draw_textf(0,0,0, "%sCh%d", buf, track.active_channel+1);
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


