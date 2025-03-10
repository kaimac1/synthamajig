#pragma once
#include "synth_common.h"
#include "instrument.hpp"

#define DEFAULT_BPM 120
#define DEFAULT_VOLUME 50

#define MAX_VOLUME_LEVEL 4096
#define AMPLITUDE_LIMIT 8192
#define NUM_VOICES 4
#define PATTERN_MAX_LEN 64
#define GATE_LENGTH_BITS 7

typedef struct {
    Note note;
    uint8_t on;
    uint8_t gate_length;
} PatternStep;

typedef struct {
    PatternStep step[PATTERN_MAX_LEN];
    int length;
} Pattern;

// Note data with a start and end time, sent to instrument
typedef struct {
    uint32_t on_time;
    uint32_t off_time;
    Note note;
} ScheduledNote;



class Voice {
public:
    Voice();
    void play(bool start);
    void schedule();

    Instrument *inst;
    Pattern pattern;
    int step;
    ScheduledNote next_note;
    
private:
    int next_note_idx();
    uint32_t next_step_time;
    bool step_on;
    bool first_step;
};



class Track {
public:
    void reset();

    // Start/stop
    void play(bool start_playing);

    void control_active_voice(InputState *input);

    // Call frequently to ensure the next notes in the pattern are scheduled
    void schedule();

    void fill_buffer(AudioBuffer buffer);

    void set_volume_percent(int vol);
    
    int bpm;
    bool is_playing;
    
    Voice voice[NUM_VOICES];
    int active_voice;

private:
    int bpm_old;
    int volume;
};
