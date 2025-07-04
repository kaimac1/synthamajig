#include "input.h"
#include <stdio.h>

RawInput input_read(void) {
    RawInput input;

    for (int i=0; i<NUM_KNOBS; i++) {
        input.knob_raw[i] = read_knob(i);
    }

    hw_scan_buttons();
    for (int i=0; i<NUM_BUTTONS; i++) {
        input.button_raw[i] = hw_read_button(i);
    }

    return input;
}

static int update_button_state(int state, bool down) {
    int new = state;

    switch (state) {
    case BTN_UP:
        if (down) new = BTN_PRESSED;
        break;
    case BTN_PRESSED:
        new = down ? BTN_DOWN : BTN_RELEASED;
        break;
    case BTN_DOWN:
        if (!down) new = BTN_RELEASED;
        break;
    case BTN_RELEASED:
        new = down ? BTN_PRESSED : BTN_UP;
        break;
    }
    return new;
}

bool input_process(InputState *input_state, RawInput in) {
    bool changed = false;

    for (int i=0; i<NUM_KNOBS; i++) {
        input_state->knob_delta[i] = -(input_state->knob_angle[i] - in.knob_raw[i]);
        input_state->knob_angle[i] = in.knob_raw[i];
        changed |= (input_state->knob_delta[i] != 0);
    }
    for (int i=0; i<NUM_BUTTONS; i++) {
        int new_state = update_button_state(input_state->button_state[i], in.button_raw[i]);
        changed |= (new_state != input_state->button_state[i]);
        input_state->button_state[i] = new_state;
    }

    return changed;
}

bool check_pressed(InputState *input, int btn) {
    bool pressed = false;
    if (input->button_state[btn] == BTN_PRESSED) {
        pressed = true;
        input->button_state[btn] = BTN_DOWN;
    }
    return pressed;
}
