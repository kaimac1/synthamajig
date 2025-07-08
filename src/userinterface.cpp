#include "userinterface.hpp"
#include "track.hpp"
#include "sample.hpp"
#include "common.h"
#include "hw/oled.h"
#include "gfx/gfx.h"
#include <cstdio>
#include <string.h>

/*
    Our user interface is based around tinyfsm states.
    The currently active state receives an InputEvent when the inputs (buttons or knobs) have
    changed. When handling an InputEvent, states should fire a DrawEvent to themselves to redraw.
    A DrawEvent is fired automatically on state entry. States should also fire the InputEvent to
    the default handler (UIFSM::react), if desired, which handles buttons common to all modes.
    States can inherit from other states and get them to handle their input.

    The function check_pressed (used through the macro PRESSED) checks if a button is pressed,
    and *consumes* that button press if it is, to ensure that a button press is only handled once.
    So for example a state can implement a function on the PLAY button, and the default handler will
    not detect PLAY presses.
*/

FSM_INITIAL_STATE(UI::UIFSM, UI::TrackView);

// Macros for readability
#define PRESSED(btn) check_pressed(&inputs, (btn))
#define RELEASED(btn) btn_release(&inputs, (btn))
#define MOD_PRESSED(mod, btn) (btn_down(&inputs, (mod)) && btn_press(&inputs, (btn)))

// The current channel and pattern
#define CHANNEL (track.channels[track.active_channel])
#define PATTERN (CHANNEL.pattern)

// The current track/project.
// Contains channels, instruments, parameters, pattern data
// Basically everything that isn't the UI is contained in here.
Track track;



namespace UI {

// UI state:
InputState inputs {};
LEDMode led_mode {LEDS_SHOW_CHANNELS};
int brightness {DEFAULT_BRIGHTNESS};
int volume_percent {DEFAULT_VOLUME};
bool recording;
bool screensaver_active;
bool keys_pressed;
int pattern_page;           // Which page of the pattern we can currently see
Step *selected_step;
Step new_step;

void control_leds();
void draw_debug_info();
void debug_menu();


void set_brightness(int level) {
    // We use 0-10, the OLED code wants 0-255
    oled_set_brightness(25 * level);
}

int next_pattern_page(void) {
    int num_pages = (PATTERN.length + NUM_STEPKEYS-1) / NUM_STEPKEYS; // round up
    return (pattern_page + 1) % num_pages;
}

// Get the step in the pattern corresponding to the given key.
// May return null, as the key could be beyond the end of the pattern.
Step *get_step_from_key(int key) {
    int stepidx = pattern_page*NUM_STEPKEYS + key;
    if (stepidx >= PATTERN.length) return NULL;
    return &PATTERN.step[stepidx];
}

void draw_header() {
    char buf[32] = {0};
    if (recording) strcat(buf, "Rec ");
    if (track.keyboard_enabled) strcat(buf, "Kb ");
    ngl_textf(FONT_A, 0,0,0, "Ch%d %s %s", track.active_channel+1, buf, false ? "[!]" : "");
}



/************************************************************/
// Default input handler

void UIFSM::react(InputEvent const & evt) {
    if (PRESSED(BTN_TRACK)) {
        transit<TrackView>();
    } else if (PRESSED(BTN_CHAN)) {
        transit<ChannelsOverview>();
    } else if (PRESSED(BTN_PATTERN)) {
        if (!is_in_state<PatternView>()) {
            pattern_page = 0;
            transit<PatternView>();    
        } else {
            pattern_page = next_pattern_page();
        }
        
    } else if (PRESSED(BTN_PLAY)) {
        track.play(!track.is_playing);
    } else if (PRESSED(BTN_REC)) {
        recording = !recording;
        react(DrawEvent {});
    } else if (PRESSED(BTN_MENU)) {
        transit<Screensaver>();
    } else if (PRESSED(BTN_KEYBOARD)) {
        track.enable_keyboard(!track.keyboard_enabled);
        react(DrawEvent {});
    }
}


/************************************************************/
// Track

void TrackView::react(InputEvent const & ievt) {
    react(DrawEvent {});
    UIFSM::react(ievt);
}

void TrackView::react(DrawEvent const & devt) {
    led_mode = LEDS_SHOW_CHANNELS;
    ngl_fillscreen(0);
    kmgui_gauge(0, &track.bpm, 5, 240, "$ bpm");
    draw_debug_info();
}


/************************************************************/
// Channels

void ChannelsOverview::react(InputEvent const & ievt) {
    if (!track.keyboard_enabled) {
        for (int i=0; i<NUM_CHANNELS; i++) {
            if (MOD_PRESSED(BTN_SHIFT, i)) {
                track.channels[i].mute(!track.channels[i].is_muted);
            } else if (PRESSED(i)) {
                track.active_channel = i;
                transit<ChannelView>();
                return;
            }
        }
    }

    react(DrawEvent {});
    UIFSM::react(ievt);    
}

void ChannelsOverview::react(DrawEvent const & devt) {
    led_mode = LEDS_SHOW_CHANNELS;
    ngl_fillscreen(0);
    draw_header();
}


/************************************************************/
// Channel

void ChannelView::react(DrawEvent const & devt) {
    ChannelsOverview::react(DrawEvent {});
    ngl_text(FONT_A,  32,32,0, "Chan edit");
    ngl_textf(FONT_A, 16,80,0, "Samp: %d", track.channels[track.active_channel].cur_sample_id);
}


/************************************************************/
// Pattern

void write_to_step(Step *step) {
    if (!selected_step) return;
    memcpy(step, selected_step, sizeof(Step));
    step->on = true;
}

void PatternView::react(InputEvent const & ievt) {
    bool shift = btn_down(&inputs, BTN_SHIFT);
    int btn = -1;
    for (int i=0; i<NUM_STEPKEYS; i++) {
        if (btn_press(&inputs, i)) {
            btn = i;
            break;
        }
    }

    Step *step = NULL;
    if (btn != -1) {
        step = get_step_from_key(btn);
    }

    if (track.keyboard_enabled) {
        if (btn != -1) {
            if (selected_step) {
                // Edit note of selected step
                selected_step->note.midi_note = track.last_played_midi_note;
            }
        }
    } else {
        // Select or toggle steps
        if (recording) {
            write_to_step(step);
        } else {
            if (step) {
                if (shift) {
                    step->on = !step->on;
                    step->note.trigger = step->on;
                } else {
                    selected_step = step;
                    transit<StepView>();
                    return;
                }
            }
        }
    }

    react(DrawEvent {});
    UIFSM::react(ievt);
}

void PatternView::react(DrawEvent const& devt) {
    if (track.keyboard_enabled) {
        led_mode = LEDS_SHOW_KEYBOARD;
    } else {
        led_mode = LEDS_SHOW_STEPS;
    }
    ngl_fillscreen(0);
    draw_header();
    kmgui_gauge(0, &PATTERN.length, 1, 64, "Len=$");
}


/************************************************************/
// Step

void change_step_note(int delta) {
    if (selected_step == NULL) return;
    const int min_note = 21;
    const int max_note = 80;

    // static int delta2 = 0;
    // const int sens = 4;
    // delta2 += delta;
    // int delta_out = delta2 / sens;
    // if (delta_out != 0) {
    //     delta2 -= sens*delta_out;
    // }

    int note = selected_step->note.midi_note + delta;
    if (note < min_note) note = min_note;
    if (note > max_note) note = max_note;
    selected_step->note.midi_note = note;
}

void StepView::react(InputEvent const &ievt) {
    if (inputs.knob_delta[1]) {
        transit<SampleSelector>();
        return;
    }
    if (inputs.knob_delta[0]) {
        change_step_note(inputs.knob_delta[0]);
    }

    react(DrawEvent {});
    PatternView::react(ievt);
}

void StepView::react(DrawEvent const& devt) {
    led_mode = track.keyboard_enabled ? LEDS_SHOW_KEYBOARD : LEDS_SHOW_STEPS;
    ngl_fillscreen(0);
    draw_header();
    if (selected_step == NULL) return;

    const int bw=62, bh=54;
    const nglFillColour col = FILLCOLOUR_HALF;
    int sx=0, sy=16;
    ngl_line(sx+1,sy,sx+bw-1,sy, col);
    ngl_line(sx+1,sy+bh,sx+bw-1,sy+bh, col);
    ngl_line(sx,sy+1,sx,sy+bh-1, col);
    ngl_line(sx+bw,sy+1,sx+bw,sy+bh-1, col);
    ngl_text(&font_palmbold, sx+2, sy+2, 0, "note");
    char buf[32];
    midi_note_to_str(buf, sizeof(buf), selected_step->note.midi_note);
    ngl_textf(FONT_A, sx+bw/2,sy+24,TEXT_CENTRE, "%s", buf);
    
    sx = 65; sy=16;
    ngl_line(sx+1,sy,sx+bw-1,sy, col);
    ngl_line(sx+1,sy+bh,sx+bw-1,sy+bh, col);
    ngl_line(sx,sy+1,sx,sy+bh-1, col);
    ngl_line(sx+bw,sy+1,sx+bw,sy+bh-1, col);
    ngl_text(&font_palmbold, sx+2, sy+2, 0, "sample");
    ngl_textf(FONT_A, sx+bw/2,sy+24,TEXT_CENTRE, "%d", selected_step->sample_id);

    sx = 0; sy=73;
    ngl_line(sx+1,sy,sx+bw-1,sy, col);
    ngl_line(sx+1,sy+bh,sx+bw-1,sy+bh, col);
    ngl_line(sx,sy+1,sx,sy+bh-1, col);
    ngl_line(sx+bw,sy+1,sx+bw,sy+bh-1, col);
    ngl_text(&font_palmbold, sx+2, sy+2, 0, "prob");
    ngl_textf(FONT_A, sx+8,sy+24,0, "trig=%d", selected_step->note.trigger);

    sx = 65; sy=73;
    ngl_line(sx+1,sy,sx+bw-1,sy, col);
    ngl_line(sx+1,sy+bh,sx+bw-1,sy+bh, col);
    ngl_line(sx,sy+1,sx,sy+bh-1, col);
    ngl_line(sx+bw,sy+1,sx+bw,sy+bh-1, col);
    ngl_text(&font_palmbold, sx+2, sy+2, 0, "level");

    
}


/************************************************************/
// Sample select

const int sample_browser_item_height = 12;

void draw_sample_browser(const char *title, int num_items) {
    ngl_text(FONT_A, 0,0,0, title);
}

void draw_sample_browser_item(const char *name, const char *value, int pos, bool selected) {
    const int flags = selected ? TEXT_INVERT : 0;
    const int yoffset = 16;
    const int ypos = sample_browser_item_height * pos;
    if (selected) ngl_rect(3, yoffset+ypos, 125,sample_browser_item_height, FILLCOLOUR_WHITE);
    ngl_text(&font_palmbold, 5, yoffset+ypos+2, flags, name);
    if (value) {
        ngl_text(&font_palmbold, 127, yoffset+ypos+2, flags | TEXT_ALIGN_RIGHT, value);
    }
}

void draw_sample_browser_scrollbar(int total_items, int visible_items, int top_item) {
    const int yoffset = 16;
    const int total_height = sample_browser_item_height*visible_items;
    const int bar_top = total_height * top_item/total_items;
    const int bar_height = total_height * visible_items/total_items;

    ngl_rect(0, yoffset, 2, total_height, FILLCOLOUR_HALF);
    ngl_rect(0, yoffset+bar_top-2, 2, 2, FILLCOLOUR_BLACK);
    ngl_rect(0, yoffset+bar_top+bar_height, 2, 2, FILLCOLOUR_BLACK);
    ngl_rect(0, yoffset+bar_top, 2, bar_height, FILLCOLOUR_WHITE);
}

wlListDrawFuncs sample_browser_funcs = {
    .draw_menu = draw_sample_browser,
    .draw_item = draw_sample_browser_item,
    .draw_scrollbar = draw_sample_browser_scrollbar
};

void SampleSelector::react(DrawEvent const& devt) {
    const int numsamps = SampleManager::sample_list.size();
    bool exit = false;
    
    ngl_fillscreen(0);
    wl_list_start("Samples", 8, 1, 0, &sample_browser_funcs);
    for (int i=0; i<numsamps; i++) {
        char value[32];
        SampleInfo *samp = &SampleManager::sample_list[i];
        if (wl_list_item_str(samp->name, NULL)) {
            // Item selected
            if (PRESSED(BTN_SHIFT)) {
                printf("loading sample %d...", samp->sample_id);
                perf_start(PERF_SAMPLE_LOAD);
                int error = SampleManager::load(samp->sample_id);
                int64_t tt = perf_end(PERF_SAMPLE_LOAD);
                printf("took %lld ms, err=%d\n", tt / 1000, error);
                selected_step->sample_id = samp->sample_id;
                exit = true;
            }
        }
    }
    wl_list_end();

    if (exit) {
        transit<StepView>();
    }
}


/************************************************************/
// Screensaver

void Screensaver::entry() {
    screensaver_active = true;
    react(DrawEvent {});
}
void Screensaver::exit() {
    screensaver_active = false;
}

void Screensaver::react(InputEvent const& ievt) {
    react(DrawEvent {});
    UIFSM::react(ievt);
}

void Screensaver::react(DrawEvent const& devt) {
    static float x = 24.0;
    static float y = 32.0;
    static float dx = 0.3;
    static float dy = 0.25;

    debug_menu();
    return;

    ngl_fillscreen(0);
    ngl_bitmap(x, y, dvdlogo);
    x += dx;
    y += dy;
    if (x+dvdlogo.width > NGL_DISPLAY_WIDTH) {
        x = NGL_DISPLAY_WIDTH - dvdlogo.width;
        dx = -dx;
    }
    if (x < 0) {
        x = 0;
        dx = -dx;
    }
    if (y+dvdlogo.height > NGL_DISPLAY_HEIGHT) {
        y = NGL_DISPLAY_HEIGHT - dvdlogo.height;
        dy = -dy;
    }
    if (y < 0) {
        y = 0;
        dy = -dy;
    }    
}



void init() {
    set_brightness(brightness);

    track.reset();
    track.set_volume_percent(volume_percent);

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
            p->step[i].note.midi_note = notes[i];
            p->step[i].note.trigger = 1;
            p->step[i].note.accent  = accts[i];
        }        
    }
    p->length = PATTERN_LEN;

    // drum
    p = &track.channels[2].pattern;
    p->length = 4;
    p->step[0].on = true;
    p->step[0].note.midi_note = 60;
    p->step[0].sample_id = 0;

    UIFSM::start();
}

bool process(RawInput in) {

    bool update = false;

    track.schedule();   // Run sequencer

    if (input_process(&inputs, in)) {
        update = true;
    }
    if (screensaver_active) update = true;
    if (track.is_playing) update = true; // debug for now, always redraw while playing

    // For gui widgets
    wl_update_knobs(inputs.knob_delta);

    if (update) {
        perf_start(PERF_DRAWTIME);
        UIFSM::dispatch(InputEvent {});
        perf_end(PERF_DRAWTIME);
    }

    control_leds();

    return update;
}


    //         case VIEW_INSTRUMENT:
    //             track.channels[track.active_channel].inst->draw(track.instrument_page);
    //             break;
    //         for (int i=0; i<NUM_STEPKEYS; i++) {
    //             if (PRESSED(i)) {
    //                 keys_pressed = true;
    //                 break;
    //             }
    //         }
    //         // Instrument pages
    //         if (MOD_PRESSED(BTN_CHAN, MODBTN_FILTER)) {
    //             change_view(VIEW_INSTRUMENT);
    //             track.instrument_page = INSTRUMENT_PAGE_FILTER;
    //         } else if (MOD_PRESSED(BTN_CHAN, MODBTN_AMP)) {
    //             change_view(VIEW_INSTRUMENT);
    //             track.instrument_page = INSTRUMENT_PAGE_AMP;
    //         // Record
    //         if (PRESSED(BTN_REC)) {
    //             recording = !recording;
    //         }
    //         // Keyboard
    //         if (PRESSED(BTN_KEYBOARD)) {
    //             track.enable_keyboard(!track.keyboard_enabled);
    //             keys_pressed = false;
    //         } else if (RELEASED(BTN_KEYBOARD)) {
    //             // Quick mode
    //             if (track.keyboard_enabled && keys_pressed) {
    //                 track.enable_keyboard(false);
    //             }
    //         }

    //     // Don't control instrument except in VIEW_INSTRUMENT
    //     if (view != VIEW_INSTRUMENT) {
    //         track.instrument_page = INSTRUMENT_PAGE_OFF;
    //     }
    // }

void control_leds() {
    if (led_mode != LEDS_OVERRIDDEN) {
        for (int i=0; i<NUM_STEPKEYS; i++) {
            uint8_t led = LED_OFF;
            int pidx;

            switch (led_mode) {
            case LEDS_SHOW_CHANNELS:
                // Show activity on each channel
                if (i < NUM_CHANNELS) {
                    if (track.get_channel_activity(i)) {
                        led = LED_ON;
                    } else {
                        led = LED_DIM;
                    }
                }
                break;

            case LEDS_SHOW_STEPS:
                pidx = pattern_page*NUM_STEPKEYS + i;
                if (PATTERN.step[pidx].on && pidx < PATTERN.length) led = LED_DIM;
                if (track.is_playing) {
                    if (CHANNEL.step == pidx) led = LED_ON;
                }
                break;

            case LEDS_SHOW_KEYBOARD:
                break;
            }
            
            hw_set_led(i, led);
        }
    }
}

void draw_debug_info(void) {

    //draw_textf(70,0,0, "P%02d", track.pattern_idx+1);
    ngl_textf(FONT_A, 127,0,TEXT_ALIGN_RIGHT, "%d/%d",
        track.channels[track.active_channel].step+1, track.channels[track.active_channel].pattern.length);
    
    // audio CPU usage
    int64_t time_audio_us = perf_get(PERF_AUDIO);
    float audio_percent = 100.0f * time_audio_us/(1E6 * BUFFER_TIME_SEC);
    ngl_textf(FONT_A, 0,115,0,"%.1f%%",  audio_percent);

    //float fps_display = 1E6 / perf_get(PERF_DISPLAY_UPDATE);
    //draw_textf(0,0,0,"%.1f fps", fps_display);
    ngl_textf(FONT_A, 127,115,TEXT_ALIGN_RIGHT,"draw %lld", perf_get(PERF_DRAWTIME));

    #ifdef DEBUG_AMPLITUDE
    ngl_textf(FONT_A, 64,64,TEXT_CENTRE,"(%d, %d)", samplemin, samplemax);
    samplemin = 0; samplemax = 0;
    #endif
}





















// Debug menu 

const int debug_item_height = 14;

void draw_debug_menu(const char *title, int num_items) {
    ngl_text(FONT_A, 0,0,0, title);
}

void draw_debug_menu_item(const char *name, const char *value, int pos, bool selected) {
    const int flags = selected ? TEXT_INVERT : 0;
    const int yoffset = 16;
    const int ypos = debug_item_height * pos;
    if (selected) ngl_rect(3, yoffset+ypos, 125,debug_item_height, FILLCOLOUR_WHITE);
    ngl_text(FONT_A, 5, yoffset+ypos+1, flags, name);
    if (value) {
        ngl_text(FONT_A, 127, yoffset+ypos+1, flags | TEXT_ALIGN_RIGHT, value);
    }
}

void draw_debug_menu_scrollbar(int total_items, int visible_items, int top_item) {
    const int yoffset = 16;
    const int total_height = debug_item_height*visible_items;
    const int bar_top = total_height * top_item/total_items;
    const int bar_height = total_height * visible_items/total_items;

    ngl_rect(0, yoffset, 2, total_height, FILLCOLOUR_HALF);
    ngl_rect(0, yoffset+bar_top-2, 2, 2, FILLCOLOUR_BLACK);
    ngl_rect(0, yoffset+bar_top+bar_height, 2, 2, FILLCOLOUR_BLACK);
    ngl_rect(0, yoffset+bar_top, 2, bar_height, FILLCOLOUR_WHITE);
}

wlListDrawFuncs debug_menu_funcs = {
    .draw_menu = draw_debug_menu,
    .draw_item = draw_debug_menu_item,
    .draw_scrollbar = draw_debug_menu_scrollbar
};

void debug_menu() {

    wl_list_start("Debug menu", 6, 0, 1, &debug_menu_funcs);
    if (wl_list_item_int("Volume", volume_percent)) {
        if (wl_list_edit_int(&volume_percent, 0, 100)) {
            track.set_volume_percent(volume_percent);    
        }
    }
    if (wl_list_item_int("Brightness", brightness)) {
        if (wl_list_edit_int(&brightness, 0, 10)) {
            set_brightness(brightness);
        }
    }
    const int nsamps = SampleManager::sample_list.size();
    for (int i=0; i<nsamps; i++) {
        char value[32];
        SampleInfo *samp = &SampleManager::sample_list[i];
        snprintf(value, sizeof(value), "%dK", samp->size_bytes/1024);
        wl_list_item_str(samp->name, value);
    }
    for (int i=0; i<15; i++) {
        char value[32];
        snprintf(value, sizeof(value), "wee");
        for (int n=0; n<i; n++) {
            strcat(value, "e");
        }
        wl_list_item_str(value, NULL);
    }
    wl_list_end();

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

} //namespace UI