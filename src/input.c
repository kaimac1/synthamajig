#include "input.h"
#include <stdio.h>

Input input_get(void) {

    Input input;

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

int update_button_state(int state, bool raw) {
    int new = state;
    switch (state) {
    case BTN_UP:
        if (raw) new = BTN_PRESSED;
        break;
    case BTN_PRESSED:
        new = raw ? BTN_DOWN : BTN_RELEASED;
        break;
    case BTN_DOWN:
        if (!raw) new = BTN_RELEASED;
        break;
    case BTN_RELEASED:
        new = raw ? BTN_PRESSED : BTN_UP;
        break;
    }
    return new;
}

bool input_detect_events(Input *input, Input input_prev) {

    bool changed = false;

    for (int i=0; i<NUM_KNOBS; i++) {
        input->knob_delta[i] = input->knob_raw[i] - input_prev.knob_raw[i];
        changed |= (input->knob_delta[i] != 0);
    }
    for (int i=0; i<NUM_BUTTONS; i++) {
        input->button_state[i] = update_button_state(input_prev.button_state[i], input->button_raw[i]);
        changed |= (input->button_state[i] != input_prev.button_state[i]);
    }

    return changed;
}

