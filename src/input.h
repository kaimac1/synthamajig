#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include "hw/hw.h"

typedef enum {
    BTN_UP,         // not being pressed
    BTN_PRESSED,    // just been pressed (active for 1 tick)
    BTN_DOWN,       // being pressed/held
    BTN_RELEASED    // just been released (active for 1 tick)
} ButtonState;

typedef struct {
    // Raw cumulative angle of an encoder.
    // Not a meaningful value - use knob_delta instead.
    int knob_raw[NUM_KNOBS];
    // Switch open/closed state.
    bool button_raw[NUM_BUTTONS];

    // Difference in value (+/-) since last input
    int knob_delta[NUM_KNOBS];
    // See ButtonState enum. Use to detect press/release events
    int button_state[NUM_BUTTONS];
} Input;

// Read raw values from encoders/buttons
Input input_get(void);

// Returns true if inputs have changed
// Updates knob_delta and button_state
bool input_detect_events(Input *input, Input input_prev);

inline bool btn_down(Input *input, int btn) {
    return input->button_raw[btn];
}

inline bool btn_press(Input *input, int btn) {
    return input->button_state[btn] == BTN_PRESSED;
}
inline bool btn_release(Input *input, int btn) {
    return input->button_state[btn] == BTN_RELEASED;
}


#ifdef __cplusplus
}
#endif
