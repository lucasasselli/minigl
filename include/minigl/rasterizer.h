#pragma once

#include "minigl/frame.h"
#include "minigl/object.h"
#include "minigl/texture.h"

#define MINIGL_SCANLINE
// #define MINIGL_PERSP_CORRECT
#define MINIGL_NO_DITHERING

/**
 * @brief Depth function enumeration.
 */
typedef enum {
    MINIGL_NEVER,    ///< Never passes.
    MINIGL_LESS,     ///< Passes if the incoming value is less than the stored value.
    MINIGL_GREATER,  ///< Passes if the incoming value is greater than the stored value.
    MINIGL_EQUAL     ///< Passes if the incoming value is equal to the stored value.
} minigl_depth_funct_t;

/**
 * @brief Face culling enumeration.
 */
typedef enum {
    MINIGL_FCULL_NONE,  ///< No face culling.
    MINIGL_FCULL_BACK,  ///< Cull back faces.
    MINIGL_FCULL_FRONT  ///< Cull front faces.
} minigl_face_cull_t;

/**
 * @brief Shading mode enumeration.
 */
typedef enum {
    MINIGL_SHADE_SOLID,    ///< Solid color shading.
    MINIGL_SHADE_TEX_2D,   ///< 2D texture shading.
    MINIGL_SHADE_MATERIAL  ///< Material shading.
} minigl_shade_mode_t;

/**
 * @brief Configuration state object.
 */
typedef struct {
    minigl_frame_t* frame;             ///< Frame buffer.
    minigl_tex_t dither;               ///< Dithering texture.
    minigl_shade_mode_t texture_mode;  ///< Texture mode.
    minigl_tex_t texture;              ///< Texture.
    uint8_t color;                     ///< Color.
    minigl_matgroup_t* matgroup;       ///< Material group.
} minigl_cfg_t;

/**
 * @brief Set the frame buffer.
 *
 * @param frame Pointer to the frame buffer.
 */
void minigl_set_frame(minigl_frame_t* frame);

/**
 * @brief Set the texture.
 *
 * @param t Texture to set.
 */
void minigl_set_tex(minigl_tex_t t);

/**
 * @brief Set the dithering texture.
 *
 * @param t Dithering texture to set.
 */
void minigl_set_dither(minigl_tex_t t);

/**
 * @brief Set the color.
 *
 * @param color Color to set.
 */
void minigl_set_color(uint8_t color);

/**
 * @brief Set the material group.
 *
 * @param matgroup Pointer to the material group.
 */
void minigl_set_matgroup(minigl_matgroup_t* matgroup);

/**
 * @brief Draw an object.
 *
 * @param obj Pointer to the object buffer.
 */
void minigl_draw(minigl_objbuf_t* obj);

int minigl_swap(void);

/**
 * @brief Clear the frame buffer and depth buffer.
 *
 * @param color Color to clear with.
 * @param depth Depth value to clear with.
 */
void minigl_clear(uint8_t color, int depth);
