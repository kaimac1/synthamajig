#pragma once
#include "synth_common.h"
#include "input.h"
#include "gfx/ngl.h"
#include "gfx/gfx_ext.h"

// Base class for instruments
class Instrument {
public:
    Instrument() {}
    virtual void init() {}
    virtual int32_t process() { return 0; }
    virtual void control(Input *input) {}
    virtual void draw() {}
    virtual void silence() { gate = 0; }

    Note note;
    bool gate;

};



typedef enum {
    AB_PARAM_ATTACK,
    AB_PARAM_DECAY,
    AB_PARAM_SUSTAIN,
    AB_PARAM_RELEASE,
    AB_PARAM_FILTER,
    AB_PARAM_RES,
    AB_PARAM_ENVMOD,
    AB_NUM_PARAMS
} AcidBassParam;

class AcidBass : public Instrument {
public:
    AcidBass();
    void init();
    int32_t process();
    void control(Input *input);
    void draw(void);

private:
    int param[AB_NUM_PARAMS];
    int env_mod;
    uint32_t cutoff;
    uint32_t resonance;    
    Oscillator osc;
    ADSR env;
    SVFilterInt filter;

    void draw_osc();
    void draw_filter();
    void draw_amp();
};




typedef enum {
    PARAM_TEST_FILTER,
    PARAM_TEST_RES,
    NUM_PARAM_TEST
} TestSynthParam;

class TestSynth : public Instrument {
public:
    TestSynth();
    void init();
    int32_t process();
    void control(Input *input);
    void draw(void);

private:
    int param[NUM_PARAM_TEST];
    uint32_t cutoff;
    uint32_t resonance;    
    Oscillator osc;
    ADSR env;
    SVFilterInt filter;

    void draw_osc();
    void draw_filter();
    void draw_amp();
};
