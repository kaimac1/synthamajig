#pragma once

#include <stdint.h>
#include <stdbool.h>

#define CLAMP(x, xmin, xmax) if ((x)>(xmax)) x=(xmax); else if ((x)<(xmin)) x=(xmin);
#define CLAMP127(x) CLAMP(x, 0, 127)

#define ISCALE 32768

#define PARAM_SCALE 128
#define PARAM_BITS 7

#define PARAM_MAX ((PARAM_SCALE)-1)
#define CLAMPPARAM(x) CLAMP(x, 0, PARAM_MAX)


void create_lookup_tables(void);
float exp_lookup(float arg);
extern uint32_t note_table[128];


// Note data sent from keyboard/sequencer/MIDI
struct Note {
    uint32_t freq;
    bool trigger;
    bool accent;
    bool glide;    
};



/************************************************/
// ADSR envelope

enum EnvState {
    ENV_ATTACK,
    ENV_DECAY,
    ENV_SUSTAIN,
    ENV_RELEASE
};

struct ADSR {
    // Parameters
    float attack;
    float decay;
    float sustain;
    float release;
    // State
    float level;
    EnvState state;
};

float process_adsr(ADSR *e, bool gate);
float map_attack(int param);
float map_sustain(int param);
float map_decay(int param);



/************************************************/
// State variable filter, second order (12 dB/oct)
//

struct SVFilter {
    float cutoff;
    float res;
    float lp0;
    float bp0;
    float hp0;
    float lp;
    float bp;
    float hp;
};

float process_svfilter(SVFilter *f, float in);
float svfreq_map(uint32_t param);



/************************************************/
// Oscillators

enum OscWave {
    WAVE_TRI,
    WAVE_SQUARE,
    WAVE_SAW,
    WAVE_NOISE,
    NUM_WAVE
};

struct Oscillator {
    // Parameters
    OscWave waveform;
    float modifier;
    float detune;
    float gain;
    // State
    uint32_t phase;
};

static inline float polyblep(uint32_t t, uint32_t dt) {
    
    if (t < dt) {
        float f = (float)t / dt;
        float x = f + f - f*f - 1.0f;
        return x;
    } else if (t > UINT32_MAX - dt) {
        float f = (float)((int64_t)t - UINT32_MAX) / dt;
        float x = f*f + f + f + 1.0f;
        return x;
    } else {
        return 0.0f;
    }
}

static inline float oscillator_saw(uint32_t phase, uint32_t dphase, uint32_t mod) {

    float p = (float)phase / (float)UINT32_MAX;
    float out = 2.0f*p - 1.0f;
    out -= polyblep(phase, dphase);
    return out;

}

int32_t oscillator_square(uint32_t phase, uint32_t dphase, uint32_t mod);

