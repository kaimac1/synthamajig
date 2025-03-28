#pragma once
#include "common.h"
#include <vector>

#define SAMPLE_NAME_SIZE 32
#define MAX_SAMPLES 64

#define SAMPLES_DIR "samples"

struct SampleInfo {
    int sample_id;
    size_t length;
    size_t size_bytes;
    bool is_valid;
    bool is_loaded;
    const int16_t *data;
    char name[SAMPLE_NAME_SIZE];
};


namespace SampleManager {
    extern std::vector<SampleInfo> sample_list;

    void init();

    // Read the disk and rebuild the sample list
    // Returns the total number of samples
    int build_list();

    // Load a sample's data into sample RAM
    int load(int sample_id);

    // Unload a sample from RAM
    int unload(int sample_id);

    // Fetch a single... sample from a sample
    int16_t fetch(int sample_id, int pos);
}