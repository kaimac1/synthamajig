#include "sample.hpp"
#include "hw/hw.h"
#include "common.h"
#include "libwave/libwave.h"
#include "tlsf/tlsf.h"
#include "ff.h"
#include <cstring>
#include <vector>

#define SAMPLES_SUFFIX ".wav"
#define SAMPLES_PATTERN ("*" SAMPLES_SUFFIX)

namespace SampleManager {

tlsf_t sample_ram;
std::vector<SampleInfo> sample_list;
int sample_map[MAX_SAMPLES];
int next_id = 0;

static inline SampleInfo *get_sample_info(int sample_id) {
    int idx = sample_map[sample_id];
    if (idx < 0) return NULL;
    return &sample_list[idx];
}

void init() {
    sample_ram = tlsf_create_with_pool((void*)PSRAM_BASE_ADDR, PSRAM_SIZE);
    build_list();
}
    
int build_list() {

    WaveFile wavefile;
    int num_samples = 0;

    sample_list.clear();
    for (int i=0; i<MAX_SAMPLES; i++) sample_map[i] = -1;

    // Scan samples directory for .wav files
    DIR dir;
    FILINFO finfo;
    FRESULT fr = f_findfirst(&dir, &finfo, SAMPLES_DIR, SAMPLES_PATTERN);
    while (fr == FR_OK && finfo.fname[0]) {
        const char *filename = &finfo.fname[0];

        // Get full path: samples/sampname.wav
        // Get sample name: sampname
        char full_path[128];
        snprintf(full_path, sizeof(full_path), "%s/%s", SAMPLES_DIR, filename);
        char *end = strstr(filename, SAMPLES_SUFFIX);
        *end = 0;
        const char *sample_name = filename;

        // Read file and parse header
        wave_open(&wavefile, full_path, WAVE_OPEN_READ);

        SampleInfo samp;
        samp.sample_id = next_id;
        samp.length = wave_get_length(&wavefile);
        samp.size_bytes = samp.length * 2;
        samp.data = NULL;
        samp.root_midi_note = DEFAULT_SAMPLE_ROOT_NOTE;
        strlcpy(samp.name, sample_name, sizeof(samp.name));

        wave_close(&wavefile);
        
        sample_list.push_back(samp);
        sample_map[samp.sample_id] = sample_list.size() - 1; // Map stores index of sample in list

        DEBUG_PRINTF("Added sample %s, id=%d len=%d idx=%d\n", sample_name, samp.sample_id, samp.length, sample_map[samp.sample_id]);

        next_id++;
        num_samples++;

        fr = f_findnext(&dir, &finfo);
    }

    f_closedir(&dir);
    return num_samples;
}

int load(int sample_id) {
    SampleInfo *samp = get_sample_info(sample_id);
    if (!samp) return -1;

    // Recreate full path from sample name
    char full_path[128];
    snprintf(full_path, sizeof(full_path), "%s/%s%s", SAMPLES_DIR, samp->name, SAMPLES_SUFFIX);

    // Allocate memory in sample RAM
    const size_t data_size = samp->size_bytes;
    DEBUG_PRINTF("Allocating %d bytes\n", data_size);
    void *data = tlsf_malloc(sample_ram, data_size);
    samp->data = (int16_t*)data;

    WaveFile wavefile;
    wave_open(&wavefile, full_path, WAVE_OPEN_READ);
    size_t frames_read = wave_read(&wavefile, (void*)samp->data, samp->length);
    DEBUG_PRINTF("Read %d of %d frames\n", frames_read, samp->length);
    wave_close(&wavefile);

    samp->is_loaded = true;
    return 0;
}

int unload(int sample_id) {
    SampleInfo *samp = get_sample_info(sample_id);
    if (!samp) return -1;

    if (!samp->is_loaded) {
        DEBUG_PRINTF("Tried to unload sample %d which is not loaded\n", sample_id);
        return -1;
    }
    if (!samp->data) {
        DEBUG_PRINTF("Null pointer for sample %d data\n", sample_id);
        return -1;
    }

    tlsf_free(sample_ram, (void*)samp->data);
    samp->is_loaded = false;
    return 0;
}

int16_t fetch(int sample_id, int pos) {
    SampleInfo *samp = get_sample_info(sample_id);
    if (!samp) return 0; // return silence
    if (!samp->is_loaded) return 0;

    if (pos >= samp->length) return 0;

    return samp->data[pos];
}


} // namespace Sample


