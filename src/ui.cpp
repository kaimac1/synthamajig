#include "ui.hpp"
#include "track.hpp"
#include "common.h"
#include "hw/oled.h"
#include "gfx/ngl.h"
#include "gfx/kmgui.h"
#include "keyboard.h"
#include <cstdio>
#include <string.h>

#include "assets/assets.h"

void ui_drawdebug(void);

// Contains channels, parameters, pattern data
Track track;

// Main state of the unit
typedef struct {
    int volume;             // 0-100
    int brightness;         // 0-10
    bool playing;
    bool recording;
    bool keyboard_enabled;
    bool keyboard_inhibit;
} Groovebox;
Groovebox gb = {
    .volume = DEFAULT_VOLUME,
    .brightness = 1
};

InputState gb_input;
UIState ui;

bool keys_pressed;
uint32_t last_note;         // Last played note freq, for entry into the pattern

Channel *cur_channel;         // Pointers to the currently selected channel and pattern
Pattern *cur_pattern;
int step_cursor;            // The currently selected step
int pattern_page;           // Which page of the pattern we can currently see


static void set_brightness(int level) {
    oled_set_brightness(25 * level);
}

// Change the current channel
static void set_channel(int i) {
    track.active_channel = i;
    cur_channel = &track.channels[track.active_channel];
    cur_pattern = &cur_channel->pattern;
}

static void enable_keyboard(bool en) {
    gb.keyboard_enabled = en;
    if (!en) {
        cur_channel->inst->silence();
    }
}

static int next_pattern_page(void) {
    int num_pages = (cur_pattern->length + NUM_KEYS-1) / NUM_KEYS; // round up
    return (pattern_page + 1) % num_pages;
}

// Get the step in the pattern corresponding to the given key.
// May return null, as the key could be beyond the end of the pattern.
static PatternStep *get_step_from_key(int key) {
    int stepidx = pattern_page*NUM_KEYS + key;
    if (stepidx >= cur_pattern->length) return NULL;
    return &cur_pattern->step[stepidx];
}



void track_page(void) {
    ui.leds = LEDS_SHOW_VOICES;
    kmgui_gauge(0, &track.bpm, 5, 240, "$ bpm");
}

void pattern_view(void) {
    const int xsp = 14, ysp = 16;

    ui.leds = LEDS_SHOW_STEPS;

    if (gb.keyboard_enabled) {
        // Play notes. Enter them into the pattern if in write mode

    } else {
        // Select or toggle steps

        bool shift = btn_down(&gb_input, BTN_SHIFT);
        for (int i=0; i<NUM_KEYS; i++) {
            if (gb.recording) {
                if (btn_press(&gb_input, i)) {
                    PatternStep *step = get_step_from_key(i);
                    if (step) {
                        step->on ^= 1;
                        if (!shift && step->on) {
                            step->note.freq = last_note;
                            step->note.trigger = 1;
                            step->note.accent = 0;
                        }
                    }
                }
            }
        }
    }

    // Pattern length 
    kmgui_gauge(0, &cur_pattern->length, 1, 64, "Length=$");

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

void select_channel(void) {
   for (int i=0; i<NUM_KEYS; i++) {
        int b = 0;
        if (i < NUM_CHANNELS) b = 64;
        if (i == track.active_channel) b = 255;
        hw_set_led(i, b);

        if (btn_press(&gb_input, i) && i<NUM_CHANNELS) {
            set_channel(i);
        }
    }
}

void debug_menu(void) {

    kmgui_menu_start("Debug menu");
    if (kmgui_menu_item_int(&gb.volume, "Volume", 0, 100)) {
        track.set_volume_percent(gb.volume);
    }
    if (kmgui_menu_item_int(&gb.brightness, "Brightness", 0, 10)) {
        set_brightness(gb.brightness);
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


extern const uint8_t testimg[];



void ui_init(void) {
    set_brightness(gb.brightness);

    ui.page = PAGE_TRACK;
    ui.inst_page = INSTRUMENT_PAGE_FILTER;
    ui.leds = LEDS_SHOW_VOICES;

    track.reset();
    track.bpm = 120;
    set_channel(0);

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
#define PRESSED(btn) btn_press(&gb_input, (btn))
#define RELEASED(btn) btn_release(&gb_input, (btn))
#define SHIFT_PRESSED(mod, btn) (btn_down(&gb_input, (mod)) && btn_press(&gb_input, (btn)))

bool ui_process(RawInput in_raw) {

    bool update = false;

    track.schedule();   // Run sequencer

    if (input_process(&gb_input, in_raw)) {
        update = true;

        for (int i=0; i<NUM_KEYS; i++) {
            if (PRESSED(i)) {
                keys_pressed = true;
                break;
            }
        }

        // Instrument pages
        if (SHIFT_PRESSED(BTN_VOICE, SBTN_FILTER)) {
            ui.page = PAGE_INSTRUMENT;
            ui.inst_page = INSTRUMENT_PAGE_FILTER;
        } else if (SHIFT_PRESSED(BTN_VOICE, SBTN_AMP)) {
            ui.page = PAGE_INSTRUMENT;
            ui.inst_page = INSTRUMENT_PAGE_AMP;


        } else if (PRESSED(BTN_TRACK)) {
            ui.page = PAGE_TRACK;
        } else if (PRESSED(BTN_MENU)) {
            ui.page = PAGE_DEBUG_MENU;
        } else if (PRESSED(BTN_PATTERN)) {
            if (ui.page != PAGE_PATTERN) {
                ui.page = PAGE_PATTERN;
                pattern_page = 0;
            } else {
                pattern_page = next_pattern_page();
            }
            
        }

        // Channel
        if (PRESSED(BTN_VOICE)) {
            ui.msg = MSG_SELECT_VOICE;
            gb.keyboard_inhibit = true;
        } else if (RELEASED(BTN_VOICE)) {
            ui.msg = MSG_NONE;
            gb.keyboard_inhibit = false;
        }

        // Play/stop
        if (PRESSED(BTN_PLAY)) {
            gb.playing ^= 1;
            track.play(gb.playing);
        }

        // Record
        if (PRESSED(BTN_REC)) {
            gb.recording ^= 1;
        }

        // Keyboard
        if (PRESSED(BTN_KEYBOARD)) {
            enable_keyboard(!gb.keyboard_enabled);
            keys_pressed = false;
        } else if (RELEASED(BTN_KEYBOARD)) {
            // Quick mode
            if (gb.keyboard_enabled && keys_pressed) {
                enable_keyboard(0);
            }
        }
    }

    // For gui widgets
    kmgui_update_knobs(gb_input.knob_delta);

    if (gb.playing) update = true;

    // Update could also be set on timer expiry here

    if (update) {
        perf_start(PERF_DRAWTIME);
        ngl_fillscreen(0);

        switch (ui.page) {
        case PAGE_INSTRUMENT:
            track.channels[track.active_channel].inst->draw();
            break;

        case PAGE_DEBUG_MENU:
            debug_menu();
            break;

        case PAGE_TRACK:
            track_page();
            break;
        
        case PAGE_PATTERN:
            pattern_view();
            break;
        }

        switch (ui.msg) {
        case MSG_SELECT_VOICE:
            select_channel();
            break;
        }

        ui_drawdebug();
        perf_end(PERF_DRAWTIME);
    }

    // Set step LEDs
    if (ui.msg) ui.leds = LEDS_OVERRIDDEN;
    if (ui.leds != LEDS_OVERRIDDEN) {
        for (int i=0; i<NUM_KEYS; i++) {
            uint8_t led = LED_OFF;
            
            if (ui.leds == LEDS_SHOW_VOICES) {
                // Show gates of each channel
                if (i < NUM_CHANNELS) {
                    if (track.channels[i].inst->gate) led = LED_ON;
                }
            } else if (ui.leds == LEDS_SHOW_STEPS) {
                int pidx = pattern_page*NUM_KEYS + i;
                if (cur_pattern->step[pidx].on && pidx < cur_pattern->length) led = LED_DIM;
                if (gb.playing) {
                    if (cur_channel->step == pidx) led = LED_ON;
                }
            }
            hw_set_led(i, led);
        }
    }

    return update;
}


// debug
void ui_drawdebug(void) {

    char buf[32] = {0};
    if (gb.recording) strcat(buf, "Rec ");
    if (gb.keyboard_enabled) strcat(buf, "Kb ");

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


// Play live notes on the active channel from the keys
// For low latency this is called from within the audio context
void play_notes_from_input(InputState *input) {
    static int current_key = -1;

    if (!gb.keyboard_enabled || gb.keyboard_inhibit) return;

    bool oct_dn = btn_down(input, BTN_LEFT);
    bool oct_up = btn_down(input, BTN_RIGHT);
    int shift = oct_up - oct_dn;

    int (*map)(int, int) = keymap_pentatonic_linear;

    // Triggers
    bool any_triggered = false;
    bool any_released = false;
    for (int b=0; b<NUM_KEYS; b++) {
        if (input->button_state[b] == BTN_RELEASED) any_released = true;
        if (input->button_state[b] == BTN_PRESSED) {
            any_triggered = true;
            current_key = b;
            int note = map(b, shift);
            if (note > 0) {
                uint32_t freq = note_table[note];
                cur_channel->inst->note.trigger = 1;
                cur_channel->inst->note.accent = btn_down(input, BTN_SHIFT);
                cur_channel->inst->note.glide = 0;
                cur_channel->inst->gate = 1;
                cur_channel->inst->note.freq = freq;

                // Store last played note for the sequencer
                last_note = freq;
            }
        }
    }

    if (!any_triggered && any_released) {
        for (int b=0; b<NUM_KEYS; b++) {
            if (input->button_state[b] == BTN_RELEASED) {
                if (current_key == b) {
                    cur_channel->inst->gate = 0;
                }
            }
        }
    }
}