#pragma once

typedef enum {
    PERF_CLIP,
    PERF_CULL,
    PERF_POLY,
    PERF_FRAG
} minigl_perf_event_t;

typedef union {
    int array[4];
    struct {
        int clip;
        int cull;
        int poly;
        int frag;
    };
} minigl_perf_data_t;

void minigl_perf_event(minigl_perf_event_t e);

void minigl_perf_clear(void);

minigl_perf_data_t minigl_perf_get(void);
