#pragma once

#include <cglm/cglm.h>

/**
 * @struct minigl_camera_t
 * @brief Represents a camera in MiniGL.
 *
 * This structure defines the properties of a camera, including its position,
 * orientation, and viewing direction.
 */
typedef struct {
    vec3 pos;    ///< Position of the camera in 3D space
    vec3 front;  ///< Direction the camera is facing
    vec3 up;     ///< Up direction of the camera
    float yaw;   ///< Yaw angle for camera rotation
    float pitch; ///< Pitch angle for camera rotation
} minigl_camera_t;
