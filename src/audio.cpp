#include "audio.hpp"
#include "common.h"
#include "hw/hw.h"
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


// All our audio generation happens within this (ISR) context
// - Read hardware inputs (encoders, buttons)
// - Update parameters for currently selected voice based on those inputs
// - Generate a new buffer of samples
extern "C" void audio_dma_callback(void) {
    static Input input_prev;

    perf_start(PERF_AUDIO);
    Input input = input_get();
    shared.input = input;

    if (input_detect_events(&input, input_prev)) {
        input_prev = input;
        // Change parameters via encoders
        track.control_active_voice(&input);

        play_notes_from_input(&input);
    }

    // Sample generation
    AudioBuffer buffer = get_audio_buffer();
    track.fill_buffer(buffer);
    put_audio_buffer(buffer);

    perf_end(PERF_AUDIO);
    shared.audio_done = 1;
}
