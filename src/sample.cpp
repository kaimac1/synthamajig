#include "sample.hpp"
#include "common.h"
#include "libwave/libwave.h"
#include "ff.h"
#include <cstring>
#include <vector>

#define SAMPLES_SUFFIX ".wav"
#define SAMPLES_PATTERN ("*" SAMPLES_SUFFIX)

namespace SampleManager {

std::vector<SampleDef> sample_list;
int sample_map[MAX_SAMPLES];
int next_id = 0;
    
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


        wave_open(&wavefile, full_path, WAVE_OPEN_READ);

        SampleDef samp;
        samp.sample_id = next_id;
        samp.length = wave_get_length(&wavefile);
        samp.data = NULL;
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
    // currently a no-op
    return 0;
}

int16_t fetch(int sample_id, int pos) {
    int idx = sample_map[sample_id];
    if (idx < 0) return 0;

    SampleDef *samp = &sample_list[idx];
    if (pos >= samp->length) return 0;

    return samp->data[pos];
}


} // namespace Sample


