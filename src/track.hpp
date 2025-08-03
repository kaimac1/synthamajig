#pragma once
#include "synth_common.hpp"
#include "instrument.hpp"

#define DEFAULT_BPM 120
#define NUM_CHANNELS 8
#define NUM_PATTERNS 16

#define PATTERN_MAX_LEN 64
#define GATE_LENGTH_BITS 7

struct Step {
    uint8_t midi_note;
    uint8_t gate_length;
    bool on : 1;
    bool trigger : 1;
    bool accent  : 1;
    int sample_id {-1};
};

struct ChannelPattern {
    Step step[PATTERN_MAX_LEN];
};

struct Pattern {
    int length;
    ChannelPattern chan[NUM_CHANNELS];
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
    void mute(bool mute);
    void silence();
    float process();
    void fill_buffer(uint32_t start_tick);

    ChannelType type;
    Instrument *inst;
    bool is_muted;
    int stepno;
    Step next_step;
    uint32_t next_on_time;
    uint32_t next_off_time;
    int samples_per_step;

    int cur_sample_id {-1};
    float cur_sample_pos;
    float cur_sample_ratio;

    float buffer[BUFFER_SIZE_SAMPS];

    bool step_on;
    uint32_t next_step_time;
};



class Track {
public:
    void reset();
    void play(bool start_playing);
    void control_active_channel(const InputState &input);
    void play_active_channel(const InputState &input);

    // Call frequently to ensure the next notes in the pattern are scheduled
    void schedule();

    // Fill channel buffer for channels in the mask
    void process_channels(uint32_t channel_mask);

    // Mix all channel buffers down into the given output buffer
    void downmix(AudioBuffer buffer);

    void set_volume_percent(int vol);
    void enable_keyboard(bool en);

    bool get_channel_activity(int chan);
    
    int bpm;
    int samples_per_step;
    bool is_playing;
    
    Channel channels[NUM_CHANNELS];
    int active_channel;

    Pattern pattern[NUM_PATTERNS];
    int current_pattern {0};

    InstrumentPage instrument_page {INSTRUMENT_PAGE_OFF};
    uint32_t last_played_midi_note {0};
    bool keyboard_enabled {false};
    bool keyboard_inhibited {false};


private:
    int next_note_idx(int channel);
    int bpm_old;
    float volume {0.0f};
    bool first_step;
};
