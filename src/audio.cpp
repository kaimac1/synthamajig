#include "pico/multicore.h"
#include "audio.hpp"
#include "track.hpp"
#include "common.h"

#define CORE0_CHANNEL_MASK  0b00001111
#define CORE1_CHANNEL_MASK  0b11110000

extern Track track;

void core1_main(void);
static int doorbell_core1_start;
static int doorbell_core1_finished;

// The main code and the audio callback use the same RawInput data which is read once per frame,
// but they (potentially) process this data at different rates, so they each have their own InputState.
// The audio_cb InputState is used to control/play the active channel with minimal latency.
static InputState audio_cb_input_state;

struct {
    volatile bool audio_done;
    RawInput input;
} shared;



// Start core 1
extern "C" void multicore_init(void) {
    multicore_reset_core1();
    doorbell_core1_start = multicore_doorbell_claim_unused((1 << NUM_CORES) - 1, true);
    doorbell_core1_finished = multicore_doorbell_claim_unused((1 << NUM_CORES) - 1, true);
    multicore_launch_core1(core1_main);
}

static void trigger_core1(void) {
    multicore_doorbell_set_other_core(doorbell_core1_start);
}
static void wait_for_core1(void) {
    while (!multicore_doorbell_is_set_current_core(doorbell_core1_finished)) {
        tight_loop_contents();
    }
    multicore_doorbell_clear_current_core(doorbell_core1_finished);
}


// Wait for audio callback on core 0 to finish
RawInput audio_wait(void) {
    // TODO: redo this properly
    // Wait for end of audio task
    while (!shared.audio_done);
    shared.audio_done = 0;
    return shared.input;
}


// Core 0 audio callback (DMA transfer complete ISR)
// - Read hardware inputs
// - Update parameters for current voice
// - Generate audio
extern "C" void audio_dma_callback(void) {
    RawInput input = input_read();
    AudioBuffer buffer = get_audio_buffer();

    perf_start(PERF_AUDIO);

    if (input_process(&audio_cb_input_state, input)) {
        // Change parameters via encoders
        // Play notes via keyboard
        track.control_active_channel(audio_cb_input_state);
        track.play_active_channel(audio_cb_input_state);
    }

    // Process channels on both cores
    trigger_core1();
    track.process_channels(CORE0_CHANNEL_MASK);
    wait_for_core1();
    
    // Mix channels down into output buffer
    track.downmix(buffer);

    shared.input = input;
    shared.audio_done = 1;
    
    perf_end(PERF_AUDIO);
    put_audio_buffer(buffer);
}




void core1_doorbell_irq(void) {
    if (multicore_doorbell_is_set_current_core(doorbell_core1_start)) {
        multicore_doorbell_clear_current_core(doorbell_core1_start);
        //printf("core1 start\n");
    }
}

void core1_main(void) {
    uint32_t irq = multicore_doorbell_irq_num(doorbell_core1_start);
    irq_set_exclusive_handler(irq, core1_doorbell_irq);
    irq_set_enabled(irq, true);

    while (1) {
        __wfi();
        track.process_channels(CORE1_CHANNEL_MASK);
        multicore_doorbell_set_other_core(doorbell_core1_finished);
    }
}
