#include "sample.hpp"
#include "hw/hw.h"
#include "hw/psram_spi.h"
#include "common.h"
#include "libwave/libwave.h"
#include "ff.h"
#include <cstring>
#include <vector>

#define SAMPLES_SUFFIX ".wav"
#define SAMPLES_PATTERN ("*" SAMPLES_SUFFIX)
#define FRAME_SIZE 2
#define FULL_PATH_LEN 128

namespace SampleManager {

std::vector<SampleInfo> sample_list;
int sample_map[MAX_SAMPLES];
int next_id = 0;

static inline SampleInfo *get_sample_info(int sample_id) {
    int idx = sample_map[sample_id];
    if (idx < 0) return NULL;
    return &sample_list[idx];
}

void init() {
    INIT_PRINTF("samples\n");
    build_list();
}
    
int build_list() {

    WaveFile wavefile;

    // Clear list and map
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
        char full_path[FULL_PATH_LEN];
        snprintf(full_path, sizeof(full_path), "%s/%s", SAMPLES_DIR, filename);
        char *end = strstr(filename, SAMPLES_SUFFIX);
        *end = 0;
        const char *sample_name = filename;

        // Read file and parse header
        wave_open(&wavefile, full_path, WAVE_OPEN_READ);
        bool valid = false;
        if (wave_get_num_channels(&wavefile) == 1 && wave_get_sample_size(&wavefile) == FRAME_SIZE) {
            valid = true;
        } else {
            INIT_PRINTF("  (\"%s\" not in right format)\n", sample_name);
        }

        if (valid) {
            SampleInfo samp;
            samp.sample_id = next_id;
            samp.length = wave_get_length(&wavefile);
            samp.size_bytes = samp.length * FRAME_SIZE;
            samp.addr = -1;
            samp.root_midi_note = DEFAULT_SAMPLE_ROOT_NOTE;
            samp.is_loaded = false;
            strlcpy(samp.name, sample_name, sizeof(samp.name));

            sample_list.push_back(samp);
            sample_map[samp.sample_id] = sample_list.size() - 1; // Map stores index of sample in list

            INIT_PRINTF("  %d: \"%s\", len=%d idx=%d\n", samp.sample_id, sample_name, samp.length, sample_map[samp.sample_id]);

            next_id++;
        }

        wave_close(&wavefile);
        fr = f_findnext(&dir, &finfo);
    }

    f_closedir(&dir);
    return sample_list.size();
}

int load_sample_data(const char *path, SampleInfo *samp) {
    int error = 0;
    WaveFile wavefile;

    wave_open(&wavefile, path, WAVE_OPEN_READ);

    // Read into a small buffer and write this out to RAM
    const size_t num_read_frames = 1024;
    const size_t buffer_size = num_read_frames * FRAME_SIZE;
    uint32_t buffer[buffer_size/4];

    size_t bytes_read = buffer_size;
    size_t total_bytes_read = 0;
    uint32_t addr = samp->addr;
    while (bytes_read == buffer_size) {
        bytes_read = FRAME_SIZE * wave_read(&wavefile, (void*)buffer, num_read_frames);
        total_bytes_read += bytes_read;

        // TODO: speed this up with a psram_writewords function
        // Note that the sample size in bytes may not be a whole number of 32-bit words.
        // This works because TLSF allocation sizes are aligned to 32 bits.
        for (size_t idx=0; idx<bytes_read; idx += 4) {
            psram_write32(addr, *(uint32_t*)((uint8_t*)buffer + idx));
            addr += 4;
        }
    }

    if (total_bytes_read != samp->size_bytes) {
        error = -1;
    }
    wave_close(&wavefile);
    return error;
}

int load(int sample_id) {
    SampleInfo *samp = get_sample_info(sample_id);
    if (!samp) return -1;

    if (samp->is_loaded) {
        DEBUG_PRINTF("Sample %d already loaded!\n", sample_id);
        return 0;
    }

    // Recreate full path from sample name
    char full_path[FULL_PATH_LEN];
    snprintf(full_path, sizeof(full_path), "%s/%s%s", SAMPLES_DIR, samp->name, SAMPLES_SUFFIX);

    // Allocate memory in sample RAM
    const size_t data_size = samp->size_bytes;
    int32_t addr = psram_alloc(data_size);
    if (addr < 0) {
        DEBUG_PRINTF("Allocation of %d bytes failed\n", data_size);
        return -1;
    }
    samp->addr = addr;

    if (load_sample_data(full_path, samp) == 0) {
        samp->is_loaded = true;
        return 0;
    } else {
        return -1;
    }
}

int unload(int sample_id) {
    SampleInfo *samp = get_sample_info(sample_id);
    if (!samp) return -1;

    if (!samp->is_loaded) {
        DEBUG_PRINTF("Tried to unload sample %d which is not loaded\n", sample_id);
        return -1;
    }
    if (samp->addr < 0) {
        DEBUG_PRINTF("Null pointer for sample %d data\n", sample_id);
        return -1;
    }

    psram_free(samp->addr);
    samp->is_loaded = false;
    return 0;
}

int16_t fetch(int sample_id, int pos) {
    SampleInfo *samp = get_sample_info(sample_id);
    if (!samp) return 0; // return silence
    if (!samp->is_loaded) return 0;

    if (pos >= samp->length) return 0;

    uint32_t data = psram_read32(samp->addr + FRAME_SIZE*pos) & 0xFFFF;
    return *(int16_t*)&data;
}

SampleInfo *get_info(int sample_id) {
    return get_sample_info(sample_id);
}

} // namespace Sample


