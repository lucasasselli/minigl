#include <cglm/cglm.h>

#include "frame.h"
#include "minigl.h"
#include "object.h"

#define CAMERA_FOV 60

void minigl_perf_print(void) {
    minigl_perf_data_t perf_data = minigl_perf_get();
#ifdef DEBUG_PERF
    printf("Clip count: %d\n", perf_data.clip);
    printf("Cull count: %d\n", perf_data.cull);
    printf("Poly count: %d\n", perf_data.poly);
    printf("Frag count: %d\n", perf_data.frag);
#endif
}

int main(void) {
    int ret;
    mat4 proj;

    minigl_camera_t camera;
    glm_vec3_copy((vec3){0.0f, 0.0f, 3.0f}, camera.pos);
    glm_vec3_copy((vec3){0.0f, 0.0f, -1.0f}, camera.front);
    glm_vec3_copy((vec3){0.0f, -1.0f, 0.0f}, camera.up);

    glm_perspective(glm_rad(CAMERA_FOV), ((float)SCREEN_SIZE_X) / ((float)SCREEN_SIZE_Y), 0.1f, 100.0f, proj);

    mat4 view;
    vec3 camera_center;
    glm_vec3_add(camera.pos, camera.front, camera_center);
    glm_lookat(camera.pos, camera_center, camera.up, view);

    mat4 model = GLM_MAT4_IDENTITY_INIT;
    glm_rotate_at(model, (vec3){0.0f, 0.0f, 0.0f}, glm_rad(180), (vec3){0.0f, 1.0f, 0.0f});
    glm_rotate_at(model, (vec3){0.0f, 0.0f, 0.0f}, glm_rad(40), (vec3){1.0f, 1.0f, 0.0f});

    mat4 trans;
    glm_mat4_mul(proj, view, trans);
    glm_mat4_mul(trans, model, trans);

    minigl_obj_t obj;
    ret = minigl_obj_read_file("tests/triangle.obj", &obj);
    if (ret) {
        printf("Can't find object file!\n");
        return 1;
    }

    minigl_obj_buf_t buf = minigl_obj_buf_init(100000);

    minigl_obj_to_obj_buf_trans(obj, trans, &buf);
    minigl_set_color(255);
    minigl_clear(0.0f, 1.0f);
    minigl_draw(buf);
    char out_file_name[50];

    minigl_frame_to_file(minigl_get_frame(), "simple_triangle.png");

    minigl_perf_print();

    return 0;
}
