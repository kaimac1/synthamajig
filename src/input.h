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
    // Cumulative angle of an encoder
    int knob_raw[NUM_KNOBS];
    // Switch open/closed state
    bool button_raw[NUM_BUTTONS];
} RawInput;

typedef struct {
    // Cumulative angle, needed to compute knob_delta - not a meaningful value on its own
    int knob_angle[NUM_KNOBS];
    // Difference in value (+/-) since last input
    int knob_delta[NUM_KNOBS];
    // See ButtonState enum
    int button_state[NUM_BUTTONS];
} InputState;



// Read raw values from encoders/buttons
RawInput input_read(void);

// Update input_state based on input_raw
// Returns true if input states have changed
bool input_process(InputState *input_state, RawInput input_raw);


inline bool btn_down(const InputState *input, int btn) {
    return ((input->button_state[btn] == BTN_PRESSED) || (input->button_state[btn] == BTN_DOWN));
}

inline bool btn_press(const InputState *input, int btn) {
    return input->button_state[btn] == BTN_PRESSED;
}

inline bool btn_release(const InputState *input, int btn) {
    return input->button_state[btn] == BTN_RELEASED;
}

// These functions "consume" a successful check, so subsequent checks don't fire
bool check_pressed(InputState *input, int btn);
bool check_released(InputState *input, int btn);



#ifdef __cplusplus
}
#endif
