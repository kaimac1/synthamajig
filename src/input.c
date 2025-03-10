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

    // // Simulated buttons via USB
    // int c = getchar_timeout_us(1);
    // input.button_raw[BTN_MENU] = c == '1';
    // input.button_raw[BTN_SEQUENCER] = c == '2';
    // input.button_raw[BTN_OSC] = c == 'q';
    // input.button_raw[BTN_FILTER] = c == 'w';
    // input.button_raw[BTN_AMP] = c == 'e';
    // input.button_raw[BTN_PLAYPAUSE] = c == 'p';
    // input.button_raw[BTN_STOP] = c == 'o';


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
        input_state->knob_delta[i] = input_state->knob_angle[i] - in.knob_raw[i];
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

