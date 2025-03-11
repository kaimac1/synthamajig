#pragma once
#include "input.h"
#include "instrument.hpp"

// Shifted button functions
#define SBTN_FILTER     BTN_STEP_9
#define SBTN_AMP        BTN_STEP_10

typedef enum {
    PAGE_DEBUG_MENU,
    PAGE_INSTRUMENT,
    PAGE_TRACK,
    PAGE_PATTERN
} UIPage;

typedef enum {
    MSG_NONE,
    MSG_SELECT_VOICE
} UIMessage;

typedef enum {
    LEDS_OVERRIDDEN,
    LEDS_SHOW_VOICES,
    LEDS_SHOW_STEPS
} LEDMode;

typedef struct {
    UIPage page;
    UIMessage msg;
    LEDMode leds;
} UIState;


class UI {
public:
    void init();
    bool process(RawInput in);

    void debug_menu();
    void pattern_view();
    void select_channel();
    void draw_debug_info();

    InputState inputs {};
    int brightness {1}; // 0-10
    int volume_percent {50};
    bool recording {false};
};