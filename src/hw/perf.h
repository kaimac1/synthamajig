#pragma once
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PERF_MAINLOOP,
    PERF_DISPLAY_UPDATE,
    PERF_WAIT,
    PERF_AUDIO,
    PERF_UI_UPDATE,
    PERF_CPUTIME,
    PERF_DRAWTIME,
    PERF_TEST,
    PERF_SAMPLE_LOAD,
    PERF_CHAN_CORE0,
    PERF_CHAN_CORE1,
    NUM_PERFCOUNTERS
} PerfMetric;

// Returns time (us) since the last call to perf_loop for the given metric
int64_t perf_loop(int metric);

// Starts a timer for the given metric
void perf_start(int metric);

// Ends a timer and returns the time since perf_start was called
int64_t perf_end(int metric);

int64_t perf_get(int metric);

#ifdef __cplusplus
}
#endif
