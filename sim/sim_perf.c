#include "../src/common.h"

int64_t perfcounter[NUM_PERFCOUNTERS];
int64_t metric_value[NUM_PERFCOUNTERS];

// Return microseconds since previous call to perf_loop
int64_t perf_loop(int metric) {
    int64_t now = 0;
    int64_t loop_us = 0;//absolute_time_diff_us(perfcounter[metric], now);
    perfcounter[metric] = now;
    metric_value[metric] = loop_us;
    return loop_us;
}

int64_t perf_get(int metric) {
    return metric_value[metric];
}

void perf_start(int metric) {
    perfcounter[metric] = 0;//get_absolute_time();
}
int64_t perf_end(int metric) {
    int64_t now = 0;//get_absolute_time();
    metric_value[metric] = 0;//absolute_time_diff_us(perfcounter[metric], now);
    return metric_value[metric];
}
