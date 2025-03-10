#pragma once
#include "input.h"

// Number of step keys
#define NUM_KEYS    16

// Shifted button functions
#define SBTN_FILTER     BTN_STEP_9
#define SBTN_AMP        BTN_STEP_10

typedef enum {
    INSTRUMENT_PAGE_OSC,
    INSTRUMENT_PAGE_AMP,
    INSTRUMENT_PAGE_FILTER
} InstrumentPage;

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
    InstrumentPage inst_page;
} UIState;
extern UIState ui;



void ui_init(void);

// Returns whether a redraw is needed
bool ui_process(RawInput input);

void ui_draw(void);

// Play live notes on active instrument from the keys
void play_notes_from_input(InputState *input);

