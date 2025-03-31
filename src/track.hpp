#pragma once
#include "synth_common.hpp"
#include "instrument.hpp"

#define DEFAULT_BPM 120
#define NUM_CHANNELS 4

#define PATTERN_MAX_LEN 64
#define GATE_LENGTH_BITS 7


struct Note {
    unsigned int midi_note;
    bool trigger;
    bool accent;
};

struct Step {
    Note note;
    int sample_id {-1};
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
    int sample_id {-1};
    Note note;
};

// Channels can be sample channels, where each step can be an arbitrary sample,
// or instrument channels, which play notes from a single instrument
enum ChannelType {
    CHANNEL_SAMPLE,
    CHANNEL_INSTRUMENT
};

class Channel {
public:
    Channel();
    void play(bool start);
    void schedule();
    void mute(bool mute);
    void silence();
    float process();

    ChannelType type;
    Instrument *inst;
    Pattern pattern;
    bool is_muted;
    int step;
    ScheduledNote next_note;
    int samples_per_step;

    int cur_sample_id {-1};
    int cur_sample_pos;

    
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

    bool get_channel_activity(int chan);
    
    int bpm;
    bool is_playing;
    bool is_over_limit;
    
    Channel channels[NUM_CHANNELS];
    int active_channel;

    InstrumentPage instrument_page {INSTRUMENT_PAGE_OFF};
    uint32_t last_played_midi_note {0};
    bool keyboard_enabled {false};
    bool keyboard_inhibited {false};

private:
    int bpm_old;
    float volume {0.0f};
};
