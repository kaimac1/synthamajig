#pragma once
#include "input.h"
#include "instrument.hpp"

// Shifted button functions
#define SBTN_FILTER     BTN_STEP_9
#define SBTN_AMP        BTN_STEP_10

enum UIPage {
    PAGE_DEBUG_MENU,
    PAGE_INSTRUMENT,
    PAGE_TRACK,
    PAGE_PATTERN
};

enum UIMessage {
    MSG_NONE,
    MSG_SELECT_VOICE
};

enum LEDMode {
    LEDS_OVERRIDDEN,
    LEDS_SHOW_VOICES,
    LEDS_SHOW_STEPS
};

struct UIState {
    UIPage page;
    UIMessage msg;
};


class UI {
public:
    void init();
    bool process(RawInput in);

    void debug_menu();
    void pattern_view();
    void select_channel();
    void draw_debug_info();
    void track_page();

    InputState inputs {};
    int brightness {1}; // 0-10
    int volume_percent {50};
    bool recording {false};
    LEDMode led_mode {LEDS_SHOW_VOICES};
};