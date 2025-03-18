#pragma once
#include "input.h"
#include "track.hpp"

// Shifted button functions
#define MODBTN_FILTER     BTN_STEP_9
#define MODBTN_AMP        BTN_STEP_10

enum UIView {
    VIEW_DEBUG_MENU,
    VIEW_INSTRUMENT,
    VIEW_TRACK,
    VIEW_PATTERN,
    VIEW_CHANNELS,
    VIEW_CHANNEL_EDIT,
    VIEW_STEP_EDIT
};

enum LEDMode {
    LEDS_OVERRIDDEN,
    LEDS_SHOW_CHANNELS,
    LEDS_SHOW_STEPS
};


class UI {
public:
    void init();
    bool process(RawInput in);

    void view_all_channels();
    void view_channel();
    void view_pattern();
    void view_step();
    void track_page();
    void debug_menu();
    void draw_debug_info();

    InputState inputs {};
    int brightness {1}; // 0-10
    int volume_percent {50};
    bool recording {false};
    LEDMode led_mode {LEDS_SHOW_CHANNELS};
    UIView view;
    Step *selected_step;

private:
    void channel_modes_common();
};