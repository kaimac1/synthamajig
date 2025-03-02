#pragma once
#ifdef __cplusplus
extern "C" {
#endif

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
typedef struct {
    uint32_t freq;
    bool trigger;
    bool accent;
    bool glide;    
} Note;



/************************************************/
// ADSR envelope

#define ENV_MAX INT32_MAX
#define ENV_BITS 31

typedef enum {
    ENV_ATTACK,
    ENV_DECAY,
    ENV_SUSTAIN,
    ENV_RELEASE
} EnvState;

typedef struct {
    // Parameters
    uint32_t attack;
    uint32_t decay;
    int32_t sustain;
    uint32_t release;
    // State
    int32_t level;
    EnvState state;
} ADSR;

int32_t process_adsr(ADSR *e, bool gate);
int32_t map_attack(int param);
int32_t map_sustain(int param);
int32_t map_decay(int param);



/************************************************/
// State variable filter, second order (12 dB/oct)
//

typedef struct {
    uint32_t cutoff;
    uint32_t res;
    int32_t lp0;
    int32_t bp0;
    int32_t hp0;
    int32_t lp;
    int32_t bp;
    int32_t hp;
} SVFilterInt;

int32_t process_svfilter_int(SVFilterInt *f, int32_t in);
uint32_t svfreq_map(uint32_t param);



/************************************************/
// Oscillators

typedef enum {
    WAVE_TRI,
    WAVE_SQUARE,
    WAVE_SAW,
    WAVE_NOISE,
    NUM_WAVE
} OscWave;

typedef struct {
    // Parameters
    OscWave waveform;
    float modifier;
    float detune;
    float gain;
    // State
    uint32_t phase;
} Oscillator;

static inline int32_t polyblep(uint32_t t, uint32_t dt) {

    if (t < dt) {
        int64_t f = ((int64_t)ISCALE * t) / dt;
        int32_t x = f + f - f*f/ISCALE - ISCALE;
        return x;
    } else if (t > UINT32_MAX - dt) {
        int64_t f = ((int64_t)ISCALE * ((int64_t)t - UINT32_MAX)) / dt;
        int32_t x = f*f/ISCALE + f + f + ISCALE;
        return x;
    } else {
        return 0;
    }
}

static inline int32_t oscillator_saw(uint32_t phase, uint32_t dphase, uint32_t mod) {

    uint64_t p = ((uint64_t)ISCALE * phase) >> 32;
    int32_t out = 2*p - ISCALE;
    out -= polyblep(phase, dphase);
    return out;

}

int32_t oscillator_square(uint32_t phase, uint32_t dphase, uint32_t mod);


#ifdef __cplusplus
}
#endif
