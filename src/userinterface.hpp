#pragma once
#include "input.h"
#include "instrument.hpp"

// Shifted button functions
#define SBTN_FILTER     BTN_STEP_9
#define SBTN_AMP        BTN_STEP_10

enum UIView {
    VIEW_DEBUG_MENU,
    VIEW_INSTRUMENT,
    VIEW_TRACK,
    VIEW_PATTERN,
    VIEW_CHANNELS,
    VIEW_CHANNEL_MENU
};

enum UIMessage {
    MSG_NONE,
    MSG_SELECT_VOICE
};

enum LEDMode {
    LEDS_OVERRIDDEN,
    LEDS_SHOW_CHANNELS,
    LEDS_SHOW_STEPS
};

struct UIState {
    UIMessage msg;
};


class UI {
public:
    void init();
    bool process(RawInput in);

    void channel_overview();
    void channel_menu();
    void pattern_view();
    void track_page();
    void debug_menu();
    void draw_debug_info();

    InputState inputs {};
    int brightness {1}; // 0-10
    int volume_percent {50};
    bool recording {false};
    LEDMode led_mode {LEDS_SHOW_CHANNELS};
    UIView view;

private:
    void handle_channel_modes();
};