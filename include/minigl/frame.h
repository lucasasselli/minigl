#pragma once

#include <stdint.h>

#ifdef __arm__
#include <arm_fp16.h>
typedef __fp16 fp16_t;
#else
typedef float fp16_t;
#endif

/**
 * @file frame.h
 * @brief Header file for the MiniGL frame structure and related functions.
 */

/**
 * @struct minigl_pixel_t
 * @brief Represents a single pixel in the MiniGL frame.
 */
typedef struct {
    fp16_t depth;      ///< Depth value of the pixel.
    uint8_t color;     ///< Color value of the pixel.
    uint8_t RESERVED;  // Easier to clear full word!
} minigl_pixel_t;

/**
 * @struct minigl_frame_t
 * @brief Represents a frame in MiniGL.
 */
typedef struct {
    minigl_pixel_t** data;  ///< 2D array of pixels representing the frame data.
    int size_x;             ///< Width of the frame.
    int size_y;             ///< Height of the frame.
} minigl_frame_t;

/**
 * @brief Creates a new MiniGL frame with the specified width and height.
 *
 * @param width Width of the frame.
 * @param height Height of the frame.
 * @return Pointer to the newly created frame.
 */
minigl_frame_t* minigl_frame_new(int width, int height);

#ifdef MINIGL_PNG
/**
 * @brief Writes the MiniGL frame to a file.
 *
 * @param frame Pointer to the frame to be written.
 * @param path Path to the file where the frame will be written.
 * @return 0 on success, non-zero on failure.
 */
int minigl_frame_to_file(minigl_frame_t* frame, char* path);
#endif
