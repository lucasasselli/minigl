#pragma once

#ifdef MINIGL_PNG
#include <spng.h>
#endif

#include <stdbool.h>
#include <stdint.h>

typedef enum : uint8_t {
    MINIGL_COLOR_FMT_G8,
    MINIGL_COLOR_FMT_GA8
} minigl_color_fmt_t;

typedef struct {
    uint8_t color;
    uint8_t alpha;
} minigl_pixel_ga8_t;

typedef struct {
    uint8_t color;
} minigl_pixel_g8_t;

typedef struct {
    minigl_color_fmt_t format;
    uint32_t size_x;
    uint32_t size_y;
    union {
        minigl_pixel_ga8_t** data_ga8;
        minigl_pixel_g8_t** data_g8;
    };
} minigl_tex_t;

typedef struct {
    bool force_g8;
    uint8_t alpha_color;
} minigl_tex_read_opts_t;

#define MINIGL_TEX_READ_OPTS_NONE ((minigl_tex_read_opts_t){0})

minigl_tex_t* minigl_tex_new(int size_x, int size_y, minigl_color_fmt_t format);

void minigl_tex_free(minigl_tex_t* tex);

#ifdef MINIGL_PNG
minigl_tex_t* minigl_tex_read_png(const char* path, minigl_tex_read_opts_t);
#endif

minigl_tex_t* minigl_tex_read_tex(const char* path);
