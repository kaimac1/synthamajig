#include "audio.hpp"
#include "common.h"
#include "synth_common.h"
#include "track.hpp"
#include "instrument.hpp"
#include "ui.hpp"

extern Track track;

struct {
    volatile bool audio_done;
    Input input;
} shared;

Input audio_wait(void) {
    // TODO: redo this properly
    // Wait for end of audio task
    while (!shared.audio_done);
    shared.audio_done = 0;
    return shared.input;
}

void audio_callback(AudioBuffer buffer, Input input) {
    static Input input_prev;

    perf_start(PERF_AUDIO);

    if (input_detect_events(&input, input_prev)) {
        input_prev = input;
        // Change parameters via encoders
        track.control_active_voice(&input);

        play_notes_from_input(&input);
    }

    // Sample generation
    track.fill_buffer(buffer);

    shared.input = input;
    shared.audio_done = 1;
    perf_end(PERF_AUDIO);
}

