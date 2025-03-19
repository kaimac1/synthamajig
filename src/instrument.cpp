#include <stdio.h>
#include <cstdint>
#include "instrument.hpp"

#define UPDATE_PARAM(par, delta) param[(par)] += delta; CLAMPPARAM(param[(par)]);




AcidBass::AcidBass() {
}


void AcidBass::init() {

    param[AB_PARAM_FILTER] = 32;
    param[AB_PARAM_RES] = 64;
    param[AB_PARAM_ENVMOD] = 16;
    param[AB_PARAM_ATTACK] = 4;
    param[AB_PARAM_DECAY] = 32;
    param[AB_PARAM_SUSTAIN] = 64;
    param[AB_PARAM_RELEASE] = 64;

    cutoff = param[AB_PARAM_FILTER];
    resonance = param[AB_PARAM_RES];
    env_mod = param[AB_PARAM_ENVMOD];
    env.attack = map_attack(param[AB_PARAM_ATTACK]);
    env.decay = map_decay(param[AB_PARAM_DECAY]);
    env.sustain = map_sustain(param[AB_PARAM_SUSTAIN]);
    env.release = map_decay(param[AB_PARAM_RELEASE]);
}


float AcidBass::process() {

    // // LFO (testing)
    // static Oscillator lfo;
    // uint32_t lfo_freq = 1400000;
    // lfo.phase += lfo_freq;
    // int32_t lfo_wave = oscillator_saw(lfo.phase, lfo_freq, 0);
    // // LFO goes from -ISCALE to +ISCALE    

    // Envelope retrigger
    if (gate && note.trigger) {
        note.trigger = false;
        env.state = ENV_ATTACK;
    }
    bool envgate = gate || note.glide; // ??

    // Modulation (testing)
    uint32_t freq = note.freq;
    int newcutoff = cutoff;
    int newresonance = resonance;
    // const int depth_n = 3;  // scale numerator
    // const int depth_b = 3;  // denominator bits
    // int *dest = &gain;
    // //*dest += (lfo_wave*depth_n) >> depth_b;
    //CLAMP(gain, 0, ISCALE*2);
    CLAMP(newcutoff, 0, 127);
    CLAMP(newresonance, 0, 127);

    // Oscillator
    uint32_t dphase = freq;
    osc.phase += dphase;
    float s = oscillator_saw(osc.phase, dphase, 0);

    // Envelope
    float envelope = process_adsr(&env, envgate);

    // Filter
    int32_t mod = env_mod * envelope;
    if (note.accent) mod = mod + mod;
    filter.cutoff = svfreq_map(newcutoff + mod);
    filter.res = (float)newresonance / PARAM_SCALE;
    (void)process_svfilter(&filter, s); // oversample
    s = process_svfilter(&filter, s);

    // Amp
    s *= envelope;
    return s;
}


void AcidBass::control(InstrumentPage page, InputState *in) {

    switch (page) {
    case INSTRUMENT_PAGE_OSC:
        break;

    case INSTRUMENT_PAGE_FILTER:
        UPDATE_PARAM(AB_PARAM_FILTER, in->knob_delta[0]);
        UPDATE_PARAM(AB_PARAM_RES,    in->knob_delta[1]);
        UPDATE_PARAM(AB_PARAM_ENVMOD, in->knob_delta[2]);
        if (in->knob_delta[0]) cutoff = param[AB_PARAM_FILTER];
        if (in->knob_delta[1]) resonance = param[AB_PARAM_RES];
        if (in->knob_delta[2]) env_mod = param[AB_PARAM_ENVMOD];
        break;

    case INSTRUMENT_PAGE_AMP:
        UPDATE_PARAM(AB_PARAM_ATTACK,  in->knob_delta[0]);
        UPDATE_PARAM(AB_PARAM_DECAY,   in->knob_delta[1]);
        UPDATE_PARAM(AB_PARAM_SUSTAIN, in->knob_delta[2]);
        UPDATE_PARAM(AB_PARAM_RELEASE, in->knob_delta[3]);
        if (in->knob_delta[0]) env.attack = map_attack(param[AB_PARAM_ATTACK]);
        if (in->knob_delta[1]) env.decay = map_decay(param[AB_PARAM_DECAY]);
        if (in->knob_delta[2]) env.sustain = map_sustain(param[AB_PARAM_SUSTAIN]);
        if (in->knob_delta[3]) env.release = map_decay(param[AB_PARAM_RELEASE]);
        break;
    }
}

void AcidBass::draw(InstrumentPage page) {

    switch (page) {
    case INSTRUMENT_PAGE_OSC:
        draw_osc();
        break;
    case INSTRUMENT_PAGE_FILTER:
        draw_filter();
        break;
    case INSTRUMENT_PAGE_AMP:
        draw_amp();
        break;
    }
}

void AcidBass::draw_osc(void) {
    draw_text(64,64, TEXT_CENTRE, "osc page");
}

void AcidBass::draw_filter(void) {
    draw_gauge_param(0, param[AB_PARAM_FILTER], "Cutoff");
    draw_gauge_param(1, param[AB_PARAM_RES], "Res.");
    draw_gauge_param(2, param[AB_PARAM_ENVMOD], "Env. mod.");
}

void AcidBass::draw_amp(void) {
    draw_gauge_param(0, param[AB_PARAM_ATTACK], "Attack");
    draw_gauge_param(1, param[AB_PARAM_DECAY], "Decay");
    draw_gauge_param(2, param[AB_PARAM_SUSTAIN], "Sustain");
    draw_gauge_param(3, param[AB_PARAM_RELEASE], "Release");
}



/****** test instrument ******/


TestSynth::TestSynth() {}


void TestSynth::init() {

    param[PARAM_TEST_FILTER] = 32;
    param[PARAM_TEST_RES] = 64;

    cutoff = param[PARAM_TEST_FILTER];
    resonance = param[PARAM_TEST_RES];
}


float TestSynth::process() {

    return 0.0f;

    /*int gain = ISCALE;
    CLAMP(gain, 0, ISCALE*2);

    // Oscillator
    uint32_t dphase = note.freq;
    osc.phase += dphase;
    int32_t s1 = oscillator_saw(osc.phase, dphase, 0);
    s1 = (s1 * gain) >> 15;

    // Filter
    //if (note.accent) mod = mod + mod;
    filter.cutoff = svfreq_map(cutoff);
    filter.res = resonance;
    s1 = process_svfilter_int(&filter, s1);    

    // Amp
    if (!gate) s1 = 0;
    return s1;*/
}


void TestSynth::control(InstrumentPage page, InputState *in) {

    switch (page) {
    case INSTRUMENT_PAGE_OSC:
        break;

    case INSTRUMENT_PAGE_FILTER:
        UPDATE_PARAM(PARAM_TEST_FILTER, in->knob_delta[0]);
        UPDATE_PARAM(PARAM_TEST_RES, in->knob_delta[1]);
        if (in->knob_delta[0]) cutoff = param[PARAM_TEST_FILTER];
        if (in->knob_delta[1]) resonance = param[PARAM_TEST_RES];
        break;

    case INSTRUMENT_PAGE_AMP:
        break;
    }
}

void TestSynth::draw(InstrumentPage page) {

    switch (page) {
    case INSTRUMENT_PAGE_OSC:
        draw_osc();
        break;
    case INSTRUMENT_PAGE_FILTER:
        draw_filter();
        break;
    case INSTRUMENT_PAGE_AMP:
        draw_amp();
        break;
    }
}

void TestSynth::draw_osc(void) {
    draw_text(64,64, TEXT_CENTRE, "osc page");
}

void TestSynth::draw_filter(void) {
    draw_gauge_param(0, param[PARAM_TEST_FILTER], "Cutoff");
    draw_gauge_param(1, param[PARAM_TEST_RES], "Res.");
}

void TestSynth::draw_amp(void) {
}
