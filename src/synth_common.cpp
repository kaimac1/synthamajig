#include "synth_common.hpp"
#include "common.h"
#include <math.h>

// Lookup tables
#define EXP_TABLE_SIZE 1024
float exp_table[EXP_TABLE_SIZE];
float svfreq_map_table[PARAM_SCALE];
uint32_t note_table[128];

float map_attack(int param) { return 1.0f / (100*(param+1)); }
float map_sustain(int param) { return (float)param / PARAM_SCALE; }
float map_decay(int param) {
    const float kd = 0.000014f;
    float df = (float)param / PARAM_SCALE + 0.06f;
    return kd / (df*df*df);
}


float oscillator_square(uint32_t phase, uint32_t dphase, uint32_t mod) {

    float out;
    uint32_t width = UINT32_MAX/2;
    uint32_t phase2;
    //float width = 0.5f + 0.5f*mod;
    
    if (phase < width) {
        out = -1.0f;
        phase2 = phase + (UINT32_MAX-width);
    } else {
        out = 1.0f;
        phase2 = phase - width;
    }
    out -= polyblep(phase, dphase);
    out += polyblep(phase2, dphase);
    return out;

}


// ADSR envelope generator
const float ENV_OVERSHOOT = 0.005f;
float process_adsr(ADSR *e, bool gate) {

    if (gate) {
        switch (e->state) {
            case ENV_RELEASE:
                e->state = ENV_ATTACK;
                // fall through

            case ENV_ATTACK:
                e->level += e->attack;
                if (e->level > 1.0f) {
                    e->level = 1.0f;
                    e->state = ENV_DECAY;
                }
                break;

            case ENV_DECAY:
                e->level += (e->sustain - ENV_OVERSHOOT - e->level) * e->decay;
                if (e->level < e->sustain) {
                    e->level = e->sustain;
                    e->state = ENV_SUSTAIN;
                }
                break;

            case ENV_SUSTAIN:
                e->level = e->sustain;
                break;
        }        
    } else {
        e->state = ENV_RELEASE;
        if (e->level > 0.0f) {
            e->level -= (e->level + ENV_OVERSHOOT) * e->release;
            if (e->level < 0.0f) e->level = 0.0f;
        }
    }

    return e->level;
}




// Curve for the SV filter cutoff parameter which has a nonlinear response
// 0..127 -> 0..1
float svfreq_map(uint32_t param) {
    if (param < 0) param = 0;
    if (param >= PARAM_SCALE) param = PARAM_SCALE - 1;
    return svfreq_map_table[param];
}

float process_svfilter(SVFilter *f, float in) {
    float kf = f->cutoff;
    float kq = 1.0f - 0.875f*f->res;   // Scale resonance by 7/8
    
    float lp0 = f->lp0 + (f->bp0 * kf);
    float hp0 = in - lp0 - (f->bp0 * kq);
    float bp0 = f->bp0 + (hp0 * kf);
    f->lp0 = lp0;
    f->bp0 = bp0;

    float lp1 = f->lp + (f->bp * kf);
    float hp1 = lp0 - lp1 - (f->bp * kq);
    float bp1 = f->bp + (hp1 * kf);
    f->lp = lp1;
    f->bp = bp1;
    f->hp = hp1;
    return lp1;
}




float exp_lookup(float arg) {
    int idx = arg * EXP_TABLE_SIZE;
    if (idx > EXP_TABLE_SIZE-1) {
        idx = EXP_TABLE_SIZE-1;
    } else if (idx < 0) {
        return 0.0f;
    }

    return exp_table[idx];
}

void create_lookup_tables(void) {

    // Exp map
    for (int i=0; i<EXP_TABLE_SIZE; i++) {
        float arg = (float)i/EXP_TABLE_SIZE;
        exp_table[i] = expf(arg);
    }

    // SV filter cutoff map
    for (int i=0; i<PARAM_SCALE; i++) {
        float arg = (float)i/PARAM_SCALE;
        svfreq_map_table[i] = 0.1f * arg * expf(2.1f*arg);
    }

    // MIDI note table
    const float freq[MIDI_NOTE_TABLE_LEN] = {
        0,8.6620,9.1770,9.7227,10.3009,10.9134,11.5623,12.2499,12.9783,
        13.7500,14.5676,15.4339,16.3516,17.3239,18.3540,19.4454,20.6017,21.8268,23.1247,24.4997,25.9565,
        27.5000,29.1352,30.8677,32.7032,34.6478,36.7081,38.8909,41.2034,43.6535,46.2493,48.9994,51.9131,
        55.0000,58.2705,61.7354,65.4064,69.2957,73.4162,77.7817,82.4069,87.3071,92.4986,97.9989,103.8262,
        110.0000,116.5409,123.4708,130.8128,138.5913,146.8324,155.5635,164.8138,174.6141,184.9972,195.9977,207.6523,
        220.0000,233.0819,246.9417,261.6256,277.1826,293.6648,311.1270,329.6276,349.2282,369.9944,391.9954,415.3047,
        440.0000,466.1638,493.8833,523.2511,554.3653,587.3295,622.2540,659.2551,698.4565,739.9888,783.9909,830.6094,
        880.0000,932.3275,987.7666,1046.5023,1108.7305,1174.6591,1244.5079,1318.5102,1396.9129,1479.9777,1567.9817,1661.2188,
        1760.0000,1864.6550,1975.5332,2093.0045,2217.4610,2349.3181,2489.0159,2637.0205,2793.8259,2959.9554,3135.9635,3322.4376,
        3520.0000,3729.3101,3951.0664,4186.0090,4434.9221,4698.6363,4978.0317,5274.0409,5587.6517,5919.9108,6271.9270,6644.8752,
        7040.0000,7458.6202,7902.1328,8372.0181,8869.8442,9397.2726,9956.0635,10548.0818,11175.3034,11839.8215,12543.8540,
    };
    for (int i=0; i<MIDI_NOTE_TABLE_LEN; i++) {
        note_table[i] = UINT32_MAX * (freq[i] / SAMPLE_RATE);
    }
}

uint32_t midi_note_to_freq(unsigned int midi_note) {
    if (midi_note >= MIDI_NOTE_TABLE_LEN) return 0;
    return note_table[midi_note];
}

int midi_note_to_str(char *buf, size_t bufsize, unsigned int midi_note) {
    if (midi_note < 21 || midi_note >= MIDI_NOTE_TABLE_LEN) { // 21 = A0
        snprintf(buf, bufsize, "-");
        return -1;
    }

    const char *notestr[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

    const int note = midi_note % 12;
    const int octave = midi_note / 12 - 1;
    snprintf(buf, bufsize, "%s%d", notestr[note], octave);
    return 0;
}