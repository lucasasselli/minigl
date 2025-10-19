#include "minigl/rasterizer.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "minigl/common.h"
#include "minigl/utils.h"

#ifdef MINIGL_INTERLACING
#define Y_STEP 2
#else
#define Y_STEP 1
#endif

static int interlacing_step = 0;

minigl_cfg_t cfg = {0};

void minigl_set_frame(minigl_frame_t* frame) {
    cfg.frame = frame;
}

void minigl_set_tex(minigl_tex_t t) {
    cfg.texture_mode = MINIGL_SHADE_TEX_2D;
    cfg.texture = t;
}

void minigl_set_dither(minigl_tex_t t) {
    cfg.dither = t;
}

void minigl_set_color(uint8_t color) {
    cfg.texture_mode = MINIGL_SHADE_SOLID;
    cfg.color = color;
}

void minigl_set_matgroup(minigl_matgroup_t* matgroup) {
    cfg.texture_mode = MINIGL_SHADE_MATERIAL;
    cfg.matgroup = matgroup;
}

MINIGL_INLINE float edge(vec4 a, vec4 b, vec4 c) {
    return (c[0] - a[0]) * (b[1] - a[1]) - (c[1] - a[1]) * (b[0] - a[0]);
}

MINIGL_INLINE float interpolate(vec3 w, float a, float b, float c) {
    return (w[0] * a + w[1] * b + w[2] * c);
}

MINIGL_INLINE int clampi(int x, int min, int max) {
    if (x < min) return min;
    if (x > max) return max;
    return x;
}

MINIGL_INLINE void vec2_swap(float* a, float* b) {
    glm_swapf(&a[0], &b[0]);
    glm_swapf(&a[1], &b[1]);
}

MINIGL_INLINE void vec4_swap(float* a, float* b) {
    glm_swapf(&a[0], &b[0]);
    glm_swapf(&a[1], &b[1]);
    glm_swapf(&a[2], &b[2]);
    glm_swapf(&a[3], &b[3]);
}

MINIGL_INLINE float min3_clampf(float a, float b, float c, float min, float max) {
    float x;
    x = a < b ? a : b;
    x = x < c ? x : c;

    x = x > min ? x : min;
    x = x < max ? x : max;

    return x;
}

MINIGL_INLINE float max3_clampf(float a, float b, float c, float min, float max) {
    float x;
    x = a > b ? a : b;
    x = x > c ? x : c;

    x = x > min ? x : min;
    x = x < max ? x : max;
    return x;
}

MINIGL_INLINE void set_pixel(int x, int y, float z, uint8_t color) {
    assert(!isnanf(z));

    minigl_pixel_t* p = &cfg.frame->data[y][x];

    if (z > p->depth || z < 0.0f) return;
    p->depth = z;

#ifndef MINIGL_NO_DITHERING
    // FIXME: use mask?
    p->color = (color >= cfg.dither.data_g8[y % cfg.dither.size_y][x % cfg.dither.size_x].color) ? 255 : 0;
#else
    p->color = color;
#endif
}

MINIGL_INLINE void set_pixel_tex_2d(int x, int y, float z, float uf, float vf) {
    // Scale
    uf *= (float)(cfg.texture.size_x - 1);
    vf *= (float)(cfg.texture.size_y - 1);

    int u = clampi(uf, 0, cfg.texture.size_x - 1);
    int v = clampi(vf, 0, cfg.texture.size_y - 1);

    if (cfg.texture.format == MINIGL_COLOR_FMT_GA8) {
        minigl_pixel_ga8_t p = cfg.texture.data_ga8[v][u];
        if (p.alpha == 0) return;
        set_pixel(x, y, z, p.color);
    } else {
        minigl_pixel_g8_t p = cfg.texture.data_g8[v][u];
        set_pixel(x, y, z, p.color);
    }
}

MINIGL_INLINE void scanline_slopes(float a, float b, float c, vec4 v[3], vec3 out) {
    out[0] = (b - a) / (v[1][1] - v[0][1]);
    out[1] = (c - a) / (v[2][1] - v[0][1]);
    out[2] = (c - b) / (v[2][1] - v[1][1]);
}

MINIGL_INLINE void scanline_range1(float a, float b, vec4 v[3], vec3 slope, float y, vec2 out) {
    MINIGL_UNUSED(b);
    out[0] = a + slope[0] * (y - v[0][1]);
    out[1] = a + slope[1] * (y - v[0][1]);
}

MINIGL_INLINE void scanline_range2(float a, float b, vec4 v[3], vec3 slope, float y, vec2 out) {
    out[0] = b + slope[2] * (y - v[1][1]);
    out[1] = a + slope[1] * (y - v[0][1]);
}

MINIGL_INLINE void scanline_add_slope1(vec3 slope, vec2 out) {
    out[0] += slope[0];
    out[1] += slope[1];
}

MINIGL_INLINE void scanline_add_slope2(vec3 slope, vec2 out) {
    out[0] += slope[2];
    out[1] += slope[1];
}

MINIGL_INLINE void scanline_draw(const int y, vec2 x_range, vec2 z_range, vec2 u_range, vec2 v_range, const uint8_t color, const bool use_tex) {
    ivec2 x_l;

#ifdef MINIGL_INTERLACING
    if ((y & 1) != interlacing_step) {
        return;
    }
#endif

    // Swap x_l[0] and x_l[1]
    int start_i = 0;
    int stop_i = 1;
    if (x_range[1] < x_range[0]) {
        start_i = 1;
        stop_i = 0;
    }

    // Sometimes the start and stop are very close, so round them to the nearest integer
    // to avoid black lines on polygon edges
    x_l[start_i] = floorf(x_range[start_i]);
    x_l[stop_i] = ceilf(x_range[stop_i]);

    float delta_k = 1.0f / (float)(x_l[stop_i] - x_l[start_i]);

    float z_delta = (z_range[stop_i] - z_range[start_i]) * delta_k;
    float u_delta = (u_range[stop_i] - u_range[start_i]) * delta_k;
    float v_delta = (v_range[stop_i] - v_range[start_i]) * delta_k;

    // Clamp AFTER calculating slopes!
    glm_ivec2_clamp(x_l, 0, cfg.frame->size_x);

    float x_start_delta = fabs(x_l[start_i] - x_range[start_i]);
    float z = z_range[start_i] + x_start_delta * z_delta;
    float u = u_range[start_i] + x_start_delta * u_delta;
    float v = v_range[start_i] + x_start_delta * v_delta;

    for (int x = x_l[start_i]; x < x_l[stop_i]; x++) {
        assert(x_l[start_i] != x_l[stop_i]);

#ifdef MINIGL_DEBUG_PERF
        minigl_perf_event(PERF_FRAG);
#endif

        if (use_tex) {
            set_pixel_tex_2d(x, y, z, u, v);
        } else {
            set_pixel(x, y, z, color);
        }

        z += z_delta;
        u += u_delta;
        v += v_delta;
    }
}

MINIGL_INLINE void draw(const minigl_objbuf_t* buf, const bool use_tex) {
    vec4 v[3];
    vec2 t[3];

    if (use_tex) {
        assert(buf->tcoord_size > 0);
    }

    // FIXME: Need to improve performance!!!
    for (int f = 0; f < buf->face_size; f++) {
        //---------------------------------------------------------------------------
        // Indexing
        //---------------------------------------------------------------------------

        // Get vertex coordinates
        bool drop = false;

        // FIXME: move out
        for (int i = 0; i < 3; i++) {
            glm_vec4_copy(buf->vcoord_ptr[buf->vface_ptr[f][i]], v[i]);

            if (v[i][3] <= 0) {
                drop = true;
                break;
            }

            v[i][3] = 1.0f / v[i][3];
            v[i][0] *= v[i][3];
            v[i][1] *= v[i][3];
            v[i][2] *= v[i][3];
        }

        if (drop) {
#ifdef MINIGL_DEBUG_PERF
            minigl_perf_event(PERF_CLIP);
#endif
            continue;
        }

        // Get texture coordinates
        if (use_tex) {
            glm_vec2_copy(buf->tcoord_ptr[buf->tface_ptr[f][0]], t[0]);
            glm_vec2_copy(buf->tcoord_ptr[buf->tface_ptr[f][1]], t[1]);
            glm_vec2_copy(buf->tcoord_ptr[buf->tface_ptr[f][2]], t[2]);
        }

        //---------------------------------------------------------------------------
        // Culling/Clipping
        //---------------------------------------------------------------------------

        // Trivial reject
        if ((v[0][0] < -1.0f && v[1][0] < -1.0f && v[2][0] < -1.0f) || (v[0][0] > 1.0f && v[1][0] > 1.0f && v[2][0] > 1.0f) ||
            (v[0][1] < -1.0f && v[1][1] < -1.0f && v[2][1] < -1.0f) || (v[0][1] > 1.0f && v[1][1] > 1.0f && v[2][1] > 1.0f) ||
            (v[0][2] < -0.0f && v[1][2] < -0.0f && v[2][2] < -0.0f) || (v[0][2] > 1.0f && v[1][2] > 1.0f && v[2][2] > 1.0f)) {
#ifdef MINIGL_DEBUG_PERF
            minigl_perf_event(PERF_CLIP);
#endif
            continue;
        }

        vec4 p;
        p[0] = (v[0][0] + v[1][0] + v[2][0]) / 3.0f;
        p[1] = (v[0][1] + v[1][1] + v[2][1]) / 3.0f;

        vec3 b;
        b[0] = edge(v[1], v[2], p);
        b[1] = edge(v[2], v[0], p);
        b[2] = edge(v[0], v[1], p);

        // NOTE: buf file have counter-clock wise wind order
        int cw_wind_order = (b[0] > 0.0f || b[1] > 0.0f || b[2] > 0.0f);

        // FIXME: Allow programmable backface Culling
        if (cw_wind_order) {
#ifdef MINIGL_DEBUG_PERF
            minigl_perf_event(PERF_CULL);
#endif
            continue;
        }

#ifndef MINIGL_SCANLINE
        if (cw_wind_order) {
            vec4_swap(v[0], v[2]);
            if (use_tex) {
                vec2_swap(t[0], t[2]);
            }
        }
#endif

        //---------------------------------------------------------------------------
        // Convert to Viewport
        //---------------------------------------------------------------------------

        for (int i = 0; i < 3; i++) {
            v[i][0] *= 0.5f * ((float)cfg.frame->size_x);
            v[i][0] += 0.5f * ((float)cfg.frame->size_x);
            v[i][1] *= 0.5f * ((float)cfg.frame->size_y);
            v[i][1] += 0.5f * ((float)cfg.frame->size_y);
        }

        //---------------------------------------------------------------------------
        // Rasterization
        //---------------------------------------------------------------------------

        uint8_t color;

        if (!use_tex) {
            if (cfg.texture_mode == MINIGL_SHADE_MATERIAL) {
                assert(buf->mface_ptr != NULL);
                int mat_i = buf->mface_ptr[f];
                assert(mat_i < cfg.matgroup->size);
                color = cfg.matgroup->color[buf->mface_ptr[f]];
            } else {
                color = cfg.color;
            }
        }

#ifdef MINIGL_DEBUG_PERF
        minigl_perf_event(PERF_POLY);
#endif

#ifdef MINIGL_SCANLINE
        // Sort vertices by y-coordinate
        if (v[0][1] > v[1][1]) {
            vec4_swap(v[0], v[1]);
            if (use_tex) vec2_swap(t[0], t[1]);
        }
        if (v[0][1] > v[2][1]) {
            vec4_swap(v[0], v[2]);
            if (use_tex) vec2_swap(t[0], t[2]);
        }
        if (v[1][1] > v[2][1]) {
            vec4_swap(v[1], v[2]);
            if (use_tex) vec2_swap(t[1], t[2]);
        }

        vec3 slope_x;
        vec3 slope_z;
        vec3 slope_tex_u;
        vec3 slope_tex_v;

        if (v[2][1] == v[0][1]) return;

        scanline_slopes(v[0][0], v[1][0], v[2][0], v, slope_x);
        scanline_slopes(v[0][2], v[1][2], v[2][2], v, slope_z);
        scanline_slopes(t[0][0], t[1][0], t[2][0], v, slope_tex_u);
        scanline_slopes(t[0][1], t[1][1], t[2][1], v, slope_tex_v);

        // Initialize scanline
        float y0 = glm_clamp(v[0][1], 0, cfg.frame->size_y - 1);
        float y1 = glm_clamp(v[1][1], 0, cfg.frame->size_y - 1);
        float y2 = glm_clamp(v[2][1], 0, cfg.frame->size_y - 1);
        float y = y0;

        vec2 x;
        vec2 z;
        vec2 tex_u;
        vec2 tex_v;

        // Draw scanline
        if (v[1][1] != v[0][1]) {
            scanline_range1(v[0][0], v[1][0], v, slope_x, y, x);
            scanline_range1(v[0][2], v[1][2], v, slope_z, y, z);
            scanline_range1(t[0][0], t[1][0], v, slope_tex_u, y, tex_u);
            scanline_range1(t[0][1], t[1][1], v, slope_tex_v, y, tex_v);

            for (; y <= y1; y += 1.0f) {
                scanline_draw(y, x, z, tex_u, tex_v, color, use_tex);

                scanline_add_slope1(slope_x, x);
                scanline_add_slope1(slope_z, z);
                scanline_add_slope1(slope_tex_u, tex_u);
                scanline_add_slope1(slope_tex_v, tex_v);
            }
        }

        if (v[2][1] != v[1][1]) {
            scanline_range2(v[0][0], v[1][0], v, slope_x, y, x);
            scanline_range2(v[0][2], v[1][2], v, slope_z, y, z);
            scanline_range2(t[0][0], t[1][0], v, slope_tex_u, y, tex_u);
            scanline_range2(t[0][1], t[1][1], v, slope_tex_v, y, tex_v);

            for (; y <= y2; y += 1.0f) {
                scanline_draw(y, x, z, tex_u, tex_v, color, use_tex);

                scanline_add_slope2(slope_x, x);
                scanline_add_slope2(slope_z, z);
                scanline_add_slope2(slope_tex_u, tex_u);
                scanline_add_slope2(slope_tex_v, tex_v);
            }
        }
#else
        // Calculate min bounding rectangle
        float mbr_min_x = min3_clampf(v[0][0], v[1][0], v[2][0], 0.0f, cfg.frame->size_x);
        float mbr_max_x = max3_clampf(v[0][0], v[1][0], v[2][0], 0.0f, cfg.frame->size_x);
        float mbr_min_y = min3_clampf(v[0][1], v[1][1], v[2][1], 0.0f, cfg.frame->size_y);
        float mbr_max_y = max3_clampf(v[0][1], v[1][1], v[2][1], 0.0f, cfg.frame->size_y);

        float area = edge(v[0], v[1], v[2]);

        for (int y = mbr_min_y; y < mbr_max_y; y++) {
            for (int x = mbr_min_x; x < mbr_max_x; x++) {
#ifdef MINIGL_DEBUG_PERF
                minigl_perf_event(PERF_FRAG);
#endif

                // Calculate the fragment coordinates
                vec4 p;
                p[0] = x;
                p[1] = y;

                b[0] = edge(v[1], v[2], p);
                b[1] = edge(v[2], v[0], p);
                b[2] = edge(v[0], v[1], p);

                // TODO: Make vertex order programmable
                // Check if the point belongs to the triangle
                if ((b[0] > 0.0f || b[1] > 0.0f || b[2] > 0.0f)) continue;

                // Calculate barycentric coord.
                glm_vec3_divs(b, area, b);

                // Interpolate Z
                float z = interpolate(b, v[0][2], v[1][2], v[2][2]);

                if (use_tex) {

#ifdef MINIGL_PERSP_CORRECT
                    // NOTE: https://stackoverflow.com/questions/24441631/how-exactly-does-opengl-do-perspectively-correct-linear-interpolation
                    // Interpolate W
                    for (int i = 0; i < 3; i++) {
                        b[i] *= v[i][3];
                    }

                    glm_vec3_divs(b, b[0] + b[1] + b[2], b);
#endif

                    // Interpolate texture coordinates
                    float tex_u = (int)(interpolate(b, t[0][0], t[1][0], t[2][0]);
                    float tex_v = (int)(interpolate(b, t[0][1], t[1][1], t[2][1]);

                    set_pixel_tex_2d(x, y, z, tex_u, tex_v);
                } else {
                    set_pixel(x, y, z, cfg.draw_color);
                }
            }
        }
#endif
    }
}

void minigl_draw(minigl_objbuf_t* buf) {
    if (cfg.texture_mode == MINIGL_SHADE_TEX_2D) {
        draw(buf, true);
    } else {
        draw(buf, false);
    }
}

int minigl_swap() {
#ifdef MINIGL_INTERLACING
    return interlacing_step ^= 1;
#else
    return 0;
#endif
}

void minigl_clear(uint8_t color, int depth) {
    const minigl_pixel_t p = {.color = color, .depth = depth};
    for (int y = interlacing_step; y < cfg.frame->size_y; y += Y_STEP) {
        for (int x = 0; x < cfg.frame->size_x; x++) {
            cfg.frame->data[y][x] = p;
        }
    }
}
