#include <stdio.h>
#include <cstdint>
#include "common.h"
#include "track.hpp"
#include "keyboard.h"

// Total count of elapsed samples
// at 44.1kHz this uint32 value will overflow after 27 hours
static uint32_t sampletick;

Instrument empty_inst;
AcidBass acid;
TestSynth testsynth;

int samples_between_notes;






void Track::reset() {
    bpm = DEFAULT_BPM;
    bpm_old = 0;

    channels[0].inst = &acid;
    channels[0].inst->init();
    channels[1].inst = &testsynth;
    channels[1].inst->init();
    channels[2].inst = &empty_inst;
    channels[3].inst = &empty_inst;   
    
    active_channel = 0;
    for (int v=0; v<NUM_CHANNELS; v++) {
        channels[v].next_note.off_time = 1;
    }
}

void Track::play(bool start_playing) {
    is_playing = start_playing;
    for (int v=0; v<NUM_CHANNELS; v++) {
        channels[v].play(start_playing);
    } 
}

void Track::set_volume_percent(int vol) {
    if (vol < 0) vol = 0;
    if (vol > 100) vol = 100;

    volume = (vol/100.0f) * MAX_VOLUME_LEVEL;
}

void Track::enable_keyboard(bool en) {
    keyboard_enabled = en;
    if (!en) {
        channels[active_channel].inst->silence();
    }    
}

void Track::schedule() {
    // BPM change
    if (bpm != bpm_old) {
        samples_between_notes = SAMPLE_RATE * 60.0f / (bpm * 4); // steps (sixteenth notes)
        bpm_old = bpm;
    }

    // Schedule next note for each channel
    if (is_playing) {
        for (int v=0; v<NUM_CHANNELS; v++) {
            channels[v].schedule();
        }
    }
}

void Track::control_active_channel(InputState *input) {
    if (channels[active_channel].inst) {
        channels[active_channel].inst->control(instrument_page, input);
    }
}

void Track::play_active_channel(InputState *input) {
    static int current_key = -1;

    if (!keyboard_enabled || keyboard_inhibited) return;

    bool oct_dn = btn_down(input, BTN_LEFT);
    bool oct_up = btn_down(input, BTN_RIGHT);
    int shift = oct_up - oct_dn;

    int (*map)(int, int) = keymap_pentatonic_linear;

    // Triggers
    bool any_triggered = false;
    bool any_released = false;
    for (int b=0; b<NUM_STEPKEYS; b++) {
        if (input->button_state[b] == BTN_RELEASED) any_released = true;
        if (input->button_state[b] == BTN_PRESSED) {
            any_triggered = true;
            current_key = b;
            int note = map(b, shift);
            if (note > 0) {
                uint32_t freq = note_table[note];
                channels[active_channel].inst->note.trigger = 1;
                channels[active_channel].inst->note.accent = btn_down(input, BTN_SHIFT);
                channels[active_channel].inst->note.glide = 0;
                channels[active_channel].inst->gate = 1;
                channels[active_channel].inst->note.freq = freq;

                // Store last played note for the sequencer
                last_played_freq = freq;
            }
        }
    }

    if (!any_triggered && any_released) {
        for (int b=0; b<NUM_STEPKEYS; b++) {
            if (input->button_state[b] == BTN_RELEASED) {
                if (current_key == b) {
                    channels[active_channel].inst->gate = 0;
                }
            }
        }
    }
}

void Track::fill_buffer(AudioBuffer buffer) {
    int16_t *samples = (int16_t *) buffer.samples;
    int count = buffer.sample_count;

    // See if note events will occur during this buffer
    //bool note_events = false;
    //if (voice[0].next_note.on_time >= sampletick && voice[0].next_note.on_time < sampletick+count) note_events = true;
    //if (voice[0].next_note.off_time >= sampletick && voice[0].next_note.off_time < sampletick+count) note_events = true;

    for (int sn=0; sn<count; sn++) {
        // TODO: see if it's faster to fill a whole buffer for each instrument separately,
        // then combine

        int32_t sample = 0;
        for (int v=0; v<NUM_CHANNELS; v++) {
            if (sampletick == channels[v].next_note.on_time) {
                channels[v].inst->gate = channels[v].next_note.note.trigger; // NOTE: trigger becomes gate
                channels[v].inst->note = channels[v].next_note.note;
            } else if (sampletick == channels[v].next_note.off_time) {
                channels[v].inst->gate = 0;
                channels[v].inst->note.trigger = 0;
            }

            sample += channels[v].inst->process();
        }

        sample = ((int64_t)(sample) * (int32_t)(volume)) >> 16;
        
        // TODO: a proper limiter
        int vlimit = AMPLITUDE_LIMIT;
        if (vlimit > 0x8000) vlimit = 0x8000;
        if (sample > (vlimit-1)) { sample = (vlimit-1); } 
        else if (sample < -vlimit) { sample = -vlimit; }
        samples[sn] = sample;
        sampletick++;
    }
}






Channel::Channel() {}

int Channel::next_note_idx() {
    if (pattern.length == 0) return 0;
    return (step + 1) % pattern.length;
}

void Channel::schedule() {

    if (!step_on && sampletick > next_step_time) {
        // Increment step
        if (!first_step) step = next_note_idx();

        // Now that we are in the "next" step, and it has started,
        // we can set the on_time for the next step
        next_step_time += samples_between_notes;
        int nn = next_note_idx();
        if (pattern.step[nn].on) {
            next_note.on_time = next_step_time;        
            next_note.note = pattern.step[nn].note;
            step_on = true;
        }
        first_step = false;
    } else if (step_on && sampletick > next_note.off_time) {
        // Step off time reached, set off time for the next step
        int nn = next_note_idx();
        int len = (samples_between_notes * pattern.step[nn].gate_length) >> GATE_LENGTH_BITS;
        next_note.off_time = next_note.on_time + len;
        step_on = false;
    }
}

void Channel::play(bool start) {

    if (start) {
        step = 0;
        next_step_time = sampletick;
        step_on = false;
        first_step = true;

        // Play first note immediately
        next_note.on_time = sampletick;
        int len = (samples_between_notes * pattern.step[step].gate_length) >> GATE_LENGTH_BITS;
        next_note.off_time = next_note.on_time + len;
        next_note.note = pattern.step[step].note;
    } else {
        step = 0;
        next_note.on_time = 0;
        next_note.off_time = sampletick;
    }

}