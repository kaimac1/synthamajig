#pragma once
#include "common.h"

#define SAMPLE_NAME_SIZE 32
#define MAX_SAMPLES 64

#define SAMPLES_DIR "samples"

struct SampleDef {
    int sample_id;
    unsigned int length;
    const int16_t *data;
    char name[SAMPLE_NAME_SIZE];
};


namespace SampleManager {
    // Read the disk and rebuild the sample list
    // Returns the total number of samples
    int build_list();

    // Fetch a single... sample from a sample
    int16_t fetch(int sample_id, int pos);
}