#pragma once
#include "common.h"

#define SAMPLE_NAME_MAXLEN 16
#define MAX_SAMPLES 64

struct SampleDef {
    int sample_id;
    int length;
    const int16_t *data;
    char name[SAMPLE_NAME_MAXLEN];
};


namespace Sample {
    // Read the disk and rebuild the sample list
    // Returns the total number of samples
    int build_list();

    // Fetch a single... sample from a sample
    int16_t fetch(int sample_id, int pos);
}