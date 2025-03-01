#pragma once
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PERF_MAINLOOP,
    PERF_DISPLAY_UPDATE,
    PERF_WAIT,
    PERF_AUDIO,
    PERF_CPUTIME,
    PERF_MATRIX,
    PERF_DRAWTIME,
    PERF_OLED_WRITETIME,
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
