#include <stdio.h>
#include <cstdint>
#include "common.h"
#include "track.hpp"

// Total count of elapsed samples
// at 44.1kHz this uint32 value will overflow after 27 hours
uint32_t sampletick;

#ifdef DEBUG_AMPLITUDE
int32_t samplemin, samplemax;
#endif

Instrument empty_inst;
AcidBass acid;
TestSynth testsynth;

int samples_between_notes;






void Track::reset() {
    bpm = DEFAULT_BPM;
    bpm_old = 0;
    set_volume_percent(DEFAULT_VOLUME);

    voice[0].inst = &acid;
    voice[0].inst->init();
    voice[1].inst = &testsynth;
    voice[1].inst->init();
    voice[2].inst = &empty_inst;
    voice[3].inst = &empty_inst;   
    
    active_voice = 0;
    for (int v=0; v<NUM_VOICES; v++) {
        voice[v].next_note.off_time = 1;
    }
}

void Track::play(bool start_playing) {
    is_playing = start_playing;
    for (int v=0; v<NUM_VOICES; v++) {
        voice[v].play(start_playing);
    } 
}

void Track::set_volume_percent(int vol) {
    if (vol < 0) vol = 0;
    if (vol > 100) vol = 100;

    volume = (vol/100.0f) * MAX_VOLUME_LEVEL;
}

void Track::schedule() {
    // BPM change
    if (bpm != bpm_old) {
        samples_between_notes = SAMPLE_RATE * 60.0f / (bpm * 4); // steps (sixteenth notes)
        bpm_old = bpm;
    }

    // Schedule next note for each voice
    if (is_playing) {
        for (int v=0; v<NUM_VOICES; v++) {
            voice[v].schedule();
        }
    }
}

void Track::control_active_voice(Input *input) {
    if (voice[active_voice].inst) {
        voice[active_voice].inst->control(input);
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
        for (int v=0; v<NUM_VOICES; v++) {
            if (sampletick == voice[v].next_note.on_time) {
                voice[v].inst->gate = voice[v].next_note.note.trigger; // NOTE: trigger becomes gate
                voice[v].inst->note = voice[v].next_note.note;
            } else if (sampletick == voice[v].next_note.off_time) {
                voice[v].inst->gate = 0;
                voice[v].inst->note.trigger = 0;
            }

            sample += voice[v].inst->process();
        }


        #ifdef DEBUG_AMPLITUDE
        if (sample < samplemin) samplemin = sample;
        if (sample > samplemax) samplemax = sample;
        #endif

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






Voice::Voice() {}

int Voice::next_note_idx() {
    return (step + 1) % pattern.length;
}

void Voice::schedule() {

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

void Voice::play(bool start) {

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