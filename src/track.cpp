#include <stdio.h>
#include <cstdint>
#include <cstring>
#include "common.h"
#include "track.hpp"
#include "keyboard.h"
#include "sample.hpp"
#include "hw/psram_spi.h"

// Total count of elapsed samples
// at 48 kHz this uint32 value will overflow after 24 hours
static uint32_t sampletick;

AcidBass acid;
TestSynth testsynth;


void Track::reset() {
    bpm = DEFAULT_BPM;
    bpm_old = 0;

    channels[0].type = CHANNEL_INSTRUMENT;
    channels[0].inst = &acid;
    channels[0].inst->init();

    channels[1].type = CHANNEL_INSTRUMENT;
    channels[1].inst = &testsynth;
    channels[1].inst->init();

    channels[2].type = CHANNEL_SAMPLE;
    channels[3].type = CHANNEL_SAMPLE;  
    
    active_channel = 0;
    for (int v=0; v<NUM_CHANNELS; v++) {
        channels[v].next_off_time = 1;
    }

    step_data.init();
}

void Track::set_volume_percent(int vol) {
    if (vol < 0) vol = 0;
    if (vol > 100) vol = 100;
    volume = vol/100.0f;
}

void Track::enable_keyboard(bool en) {
    keyboard_enabled = en;
    if (!en) {
        channels[active_channel].silence();
    }    
}

void Track::play(bool start_playing) {
    is_playing = start_playing;

    for (int v=0; v<NUM_CHANNELS; v++) {
        Channel *c = &channels[v];

        if (start_playing) {
            c->stepno = 0;
            c->next_step_time = sampletick;
            c->step_on = false;
            first_step = true;

            // Play first note immediately
            const Step first_step = step_data.get_step(current_pattern, v, c->stepno);
            if (first_step.on) {
                c->next_on_time = sampletick;
                int len = (samples_per_step * first_step.gate_length) >> GATE_LENGTH_BITS;
                c->next_off_time = c->next_on_time + len;
                c->next_step = first_step;
            }
        } else {
            c->stepno = 0;
            c->next_on_time = 0;
            c->next_off_time = sampletick;
        }
    }
}

void Track::schedule() {
    // Tempo change
    if (bpm != bpm_old) {
        samples_per_step = SAMPLE_RATE * 60.0f / (bpm * 4); // steps (sixteenth notes)
        bpm_old = bpm;
    }

    // Schedule next note for each channel
    if (is_playing) {
        for (int v=0; v<NUM_CHANNELS; v++) {
            Channel *c = &channels[v];

            if (!c->step_on && sampletick > c->next_step_time) {
                // Increment step
                if (!first_step) c->stepno = next_note_idx(v);

                // Now that we are in the "next" step, and it has started,
                // we can set the on_time for the next step
                c->next_step_time += samples_per_step;
                int nn = next_note_idx(v);
                const Step next_step = step_data.get_step(current_pattern, v, nn);
                if (next_step.on) {
                    c->next_on_time = c->next_step_time;
                    c->next_step = next_step;
                    c->step_on = true;
                }
            } else if (c->step_on && sampletick > c->next_off_time) {
                // Step off time reached, set off time for the next step
                int nn = next_note_idx(v);
                const Step next_step = step_data.get_step(current_pattern, v, nn);
                int len = (samples_per_step * next_step.gate_length) >> GATE_LENGTH_BITS;
                c->next_off_time = c->next_on_time + len;
                c->step_on = false;
            }
        }
        first_step = false;
    }
}

int Track::next_note_idx(int channel) {
    if (pattern[current_pattern].length == 0) return 0;
    return (channels[channel].stepno + 1) % pattern[current_pattern].length;
}

bool Track::get_channel_activity(int chan) {
    if (channels[chan].type == CHANNEL_INSTRUMENT) {
        return channels[chan].inst->gate;
    } else if (channels[chan].type == CHANNEL_SAMPLE) {
        return (channels[chan].cur_sample_id >= 0);
    }
    return false;
}

void Track::control_active_channel(const InputState &input) {
    if (channels[active_channel].inst) {
        channels[active_channel].inst->control(instrument_page, &input);
    }
}

void Track::play_active_channel(const InputState &input) {
    static int current_key = -1;

    if (!keyboard_enabled || keyboard_inhibited) return;

    bool oct_dn = btn_down(&input, BTN_LEFT);
    bool oct_up = btn_down(&input, BTN_RIGHT);
    int shift = oct_up - oct_dn;

    int (*map)(int, int) = keymap_pentatonic_linear;

    // Triggers
    bool any_triggered = false;
    bool any_released = false;
    for (int b=0; b<NUM_STEPKEYS; b++) {
        if (input.button_state[b] == BTN_RELEASED) any_released = true;
        if (input.button_state[b] == BTN_PRESSED) {
            any_triggered = true;
            current_key = b;
            int midi_note = map(b, shift);
            if (midi_note > 0) {
                uint32_t freq = midi_note_to_freq(midi_note);
                channels[active_channel].inst->trigger = 1;
                channels[active_channel].inst->accent = btn_down(&input, BTN_SHIFT);
                channels[active_channel].inst->gate = 1;
                channels[active_channel].inst->note_freq = freq;

                // Store last played note for the sequencer
                last_played_midi_note = midi_note;
            }
        }
    }

    if (!any_triggered && any_released) {
        for (int b=0; b<NUM_STEPKEYS; b++) {
            if (input.button_state[b] == BTN_RELEASED) {
                if (current_key == b) {
                    channels[active_channel].inst->gate = 0;
                }
            }
        }
    }
}

void Track::process_channels(uint32_t channel_mask) {

    int chan = 0;
    while (chan < NUM_CHANNELS) {
        if (channel_mask & 1) {
            if (!channels[chan].is_muted) {
                channels[chan].fill_buffer(sampletick);
            }
        }

        channel_mask >>= 1;
        chan++;
    }
}

void Track::downmix(AudioBuffer buffer) {
    int16_t *samples = (int16_t *) buffer.samples;
    memset(samples, 0, 2*BUFFER_SIZE_SAMPS);

    for (int sn=0; sn<BUFFER_SIZE_SAMPS; sn++) {
        float sample = 0.0f;

        for (int v=0; v<NUM_CHANNELS; v++) {
            if (channels[v].is_muted) continue;

            // Volume & convert to 16-bit
            sample += channels[v].buffer[sn] * volume * 0.2f * 32767;
        }

        // TODO: a proper limiter
        const float vlimit = 20000.0f;
        if (sample > vlimit) { 
            sample = vlimit;
        } 
        else if (sample < -vlimit) {
            sample = -vlimit;
        }        

        samples[sn] = (int16_t)sample;
    }

    sampletick += BUFFER_SIZE_SAMPS;

    // // See if note events will occur during this buffer
    // //bool note_events = false;
    // //if (voice[0].next_note.on_time >= sampletick && voice[0].next_note.on_time < sampletick+count) note_events = true;
    // //if (voice[0].next_note.off_time >= sampletick && voice[0].next_note.off_time < sampletick+count) note_events = true;
}




void Channel::mute(bool mute) {
    is_muted = mute;
    if (mute && type == CHANNEL_INSTRUMENT) {
        inst->gate = false;
    }
}

void Channel::silence() {
    if (type == CHANNEL_INSTRUMENT) {
        inst->silence();
    }
}

float Channel::process() {
    if (type == CHANNEL_INSTRUMENT) {
        return inst->process();

    } else if (type == CHANNEL_SAMPLE) {
        if (cur_sample_id < 0) return 0.0f;
        int16_t s = SampleManager::fetch(cur_sample_id, cur_sample_pos);
        cur_sample_pos += cur_sample_ratio;
        
        return s/32768.0f;
    }
    return 0.0f;
}

void Channel::fill_buffer(uint32_t start_tick) {
    for (int sn=0; sn<BUFFER_SIZE_SAMPS; sn++) {
        uint32_t tick = start_tick + sn;

        if (type == CHANNEL_INSTRUMENT) {
            if (tick == next_on_time) {
                // Set up instrument to play next note now
                //set_note_on_instrument(channels[v].inst, &channels[v].next_note);
                inst->gate = next_step.trigger; // NOTE: trigger becomes gate
                inst->accent = next_step.accent;
                uint32_t freq = midi_note_to_freq(next_step.midi_note);
                inst->note_freq = freq;
            } else if (tick == next_off_time) {
                inst->gate = 0;
                inst->trigger = 0;
            }

        } else if (type == CHANNEL_SAMPLE) {
            if (tick == next_on_time) {
                cur_sample_id = next_step.sample_id;
                cur_sample_pos = 0;
                if (cur_sample_id >= 0) {
                    uint32_t play_freq = midi_note_to_freq(next_step.midi_note);
                    SampleInfo *samp = SampleManager::get_info(cur_sample_id);
                    uint32_t root_freq = midi_note_to_freq(samp->root_midi_note);
                    cur_sample_ratio = (float)play_freq / root_freq;
                }
            }
        }

        buffer[sn] = process();
    }
}



void StepData::init() {
    const size_t alloc_size = sizeof(Step) * PATTERN_MAX_LEN * NUM_PATTERNS * NUM_CHANNELS;
    printf("stepdata: step_size=%d\n", sizeof(Step));
    printf("stepdata: alloc_size=%d\n", alloc_size);

    baseaddr = psram_alloc(alloc_size);

    // Initialise data
    Step step;
    step.on = false;
    for (int stepno=0; stepno<PATTERN_MAX_LEN; stepno++) {
        for (int chan=0; chan<NUM_CHANNELS; chan++) {
            for (int ptn=0; ptn<NUM_PATTERNS; ptn++) {
                set_step(ptn, chan, stepno, step);
            }
        }
    }
}

Step StepData::get_step(int pattern, int chan, int stepno) {
    const int32_t addr = baseaddr + sizeof(Step)*(PATTERN_MAX_LEN*(NUM_CHANNELS*pattern + chan) + stepno);
    
    Step step;
    psram_read(addr, (uint8_t*)&step, sizeof(Step));
    return step;
}

void StepData::set_step(int pattern, int chan, int stepno, Step step) {
    const int32_t addr = baseaddr + sizeof(Step)*(PATTERN_MAX_LEN*(NUM_CHANNELS*pattern + chan) + stepno);

    psram_write(addr, (uint8_t*)&step, sizeof(Step));
}