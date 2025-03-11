#pragma once
#include "synth_common.hpp"
#include "instrument.hpp"

#define DEFAULT_BPM 120
#define NUM_CHANNELS 4

#define MAX_VOLUME_LEVEL 4096
#define AMPLITUDE_LIMIT 8192
#define PATTERN_MAX_LEN 64
#define GATE_LENGTH_BITS 7


struct Step {
    Note note;
    bool on;
    uint8_t gate_length;
};

struct Pattern {
    Step step[PATTERN_MAX_LEN];
    int length;
};

// Note data with a start and end time, sent to instrument
struct ScheduledNote {
    uint32_t on_time;
    uint32_t off_time;
    Note note;
};

enum ChannelType {
    CHANNEL_SAMPLE,
    CHANNEL_INSTRUMENT
};



class Channel {
public:
    Channel();
    void play(bool start);
    void schedule();

    ChannelType type;
    Instrument *inst;
    Pattern pattern;
    int step;
    ScheduledNote next_note;
    int samples_per_step;
    
private:
    int next_note_idx();
    uint32_t next_step_time;
    bool step_on;
    bool first_step;
};



class Track {
public:
    void reset();
    void play(bool start_playing);
    void control_active_channel(InputState *input);
    void play_active_channel(InputState *input);
    // Call frequently to ensure the next notes in the pattern are scheduled
    void schedule();
    void fill_buffer(AudioBuffer buffer);
    void set_volume_percent(int vol);
    void enable_keyboard(bool en);
    
    int bpm;
    bool is_playing;
    
    Channel channels[NUM_CHANNELS];
    int active_channel;

    InstrumentPage instrument_page {INSTRUMENT_PAGE_FILTER};
    uint32_t last_played_freq {0};
    bool keyboard_enabled {false};
    bool keyboard_inhibited {false};

private:
    int bpm_old;
    int volume;
};
