#pragma once
#include "synth_common.hpp"
#include "input.h"
#include "gfx/gfx.h"

enum InstrumentPage {
    INSTRUMENT_PAGE_OFF,
    INSTRUMENT_PAGE_OSC,
    INSTRUMENT_PAGE_AMP,
    INSTRUMENT_PAGE_FILTER
};


// Base class for instruments
class Instrument {
public:
    Instrument() {}
    virtual void init() {}
    virtual float process() { return 0.0f; }
    virtual void control(InstrumentPage page, const InputState *input) {}
    virtual void draw(InstrumentPage page) {}
    virtual void silence() { gate = 0; }

    uint32_t note_freq;
    bool trigger;
    bool accent;
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
    float process();
    void control(InstrumentPage page, const InputState *input);
    void draw(InstrumentPage page);

private:
    int param[AB_NUM_PARAMS];
    int env_mod;
    uint32_t cutoff;
    uint32_t resonance;    
    Oscillator osc;
    ADSR env;
    SVFilter filter;

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
    float process();
    void control(InstrumentPage page, const InputState *input);
    void draw(InstrumentPage page);

private:
    int param[NUM_PARAM_TEST];
    uint32_t cutoff;
    uint32_t resonance;    
    Oscillator osc;
    ADSR env;
    SVFilter filter;

    void draw_osc();
    void draw_filter();
    void draw_amp();
};
