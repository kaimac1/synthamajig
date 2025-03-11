#include "audio.hpp"
#include "track.hpp"
#include "common.h"

extern Track track;

// The main code and the audio callback use the same RawInput data which is read once per frame,
// but they (potentially) process this data at different rates, so they each have their own InputState.
// The audio_cb InputState is used to control/play the active channel with minimal latency.
static InputState audio_cb_input_state;

struct {
    volatile bool audio_done;
    RawInput input;
} shared;



RawInput audio_wait(void) {
    // TODO: redo this properly
    // Wait for end of audio task
    while (!shared.audio_done);
    shared.audio_done = 0;
    return shared.input;
}

void audio_callback(AudioBuffer buffer, RawInput input) {
    perf_start(PERF_AUDIO);

    if (input_process(&audio_cb_input_state, input)) {
        // Change parameters via encoders
        // Play notes via keyboard
        track.control_active_channel(&audio_cb_input_state);
        track.play_active_channel(&audio_cb_input_state);
    }

    // Sample generation
    track.fill_buffer(buffer);

    shared.input = input;
    shared.audio_done = 1;
    perf_end(PERF_AUDIO);
}

