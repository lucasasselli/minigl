#include "minigl/object.h"

#include <stdlib.h>
#include <string.h>

#include "minigl/system.h"
#include "minigl/utils.h"

#define LINE_BUFFER_SIZE 70
#define MAT_NAME_SIZE_MAX 20
#define MAT_CNT_MAX 30

static bool str_starts_with(const char *a, const char *b) {
    if (strncmp(a, b, strlen(b)) == 0) return 1;
    return 0;
}

static int get_mat_enum(const char *name, const char mat_array[MAT_CNT_MAX][MAT_NAME_SIZE_MAX], int mat_cnt) {
    for (int i = 0; i < mat_cnt; i++) {
        if (strcmp(name, mat_array[i]) == 0) return i;
    }

    return mat_cnt;
}

int minigl_obj_read_file(char *path, minigl_obj_t *out, int flags) {
    // Get the file handle
    void *f = minigl_fopen(path, "r");
    if (f == NULL) {
        return 1;
    }

    char line_buffer[LINE_BUFFER_SIZE];
    int char_index = 0;

    char mat_name[MAT_NAME_SIZE_MAX];
    char mat_array[MAT_CNT_MAX][MAT_NAME_SIZE_MAX];
    int mat_cnt = 0;

    bool has_uv = false;
    bool has_mat = false;

    // Read file line by line into the buffer
    out->vcoord_size = 0;
    out->face_size = 0;
    out->tcoord_size = 0;

    char c;

    while (minigl_fread((void *)&c, 1, f)) {
        line_buffer[char_index] = c;
        if (c == '\n' || c == EOF) {
            // Found EOL
            line_buffer[char_index] = '\0';

            if (str_starts_with(line_buffer, "v ")) {
                // Vertex coordidate
                out->vcoord_size++;
            }

            if (str_starts_with(line_buffer, "vt ")) {
                // Texture coordinate
                has_uv = true;
                out->tcoord_size++;
            }

            if (str_starts_with(line_buffer, "f ")) {
                // Face
                out->face_size++;
            }

            if (str_starts_with(line_buffer, "usemtl ")) {
                // Enumerate the materials
                sscanf(line_buffer, "usemtl %s", mat_name);
                has_mat = true;
                if (get_mat_enum(mat_name, mat_array, mat_cnt) == mat_cnt) {
                    // New material
                    strcpy(mat_array[mat_cnt], mat_name);
                    mat_cnt++;
                }
            }

            // Reset line
            char_index = 0;
        } else {
            char_index++;
        }
    }

    // Allocate memory
    out->vcoord_ptr = malloc(sizeof(vec4) * out->vcoord_size);
    out->vface_ptr = malloc(sizeof(ivec3) * out->face_size);

    assert(out->face_size > 0);

    out->tcoord_ptr = malloc(sizeof(vec4) * out->tcoord_size);

    if (has_uv) {
        out->tface_ptr = malloc(sizeof(ivec3) * out->face_size);
    } else {
        out->tface_ptr = NULL;
    }

    if (has_mat) {
        out->mface_ptr = malloc(sizeof(int) * out->face_size);
    } else {
        out->mface_ptr = NULL;
    }

    // Load data
    out->vcoord_size = 0;
    out->face_size = 0;
    out->tcoord_size = 0;

    int active_mat = -1;

    minigl_fseek(f, 0, SEEK_SET);

    while (minigl_fread((void *)&c, 1, f)) {
        line_buffer[char_index] = c;
        if (c == '\n' || c == EOF) {
            // Found EOL
            line_buffer[char_index] = '\0';

            if (str_starts_with(line_buffer, "v ")) {
                // Vertex coordidate
                float x, y, z;
                sscanf(line_buffer, "v %f %f %f", &x, &y, &z);
                out->vcoord_ptr[out->vcoord_size][0] = x;
                out->vcoord_ptr[out->vcoord_size][1] = y;
                out->vcoord_ptr[out->vcoord_size][2] = z;
                out->vcoord_ptr[out->vcoord_size][3] = 1.0f;
                out->vcoord_size++;
            }

            if (str_starts_with(line_buffer, "vt ")) {
                // Texture coordinate
                float x, y;
                sscanf(line_buffer, "vt %f %f", &x, &y);
                out->tcoord_ptr[out->tcoord_size][0] = x;
                if (flags & MINIGL_OBJ_TEXFLIPY) {
                    out->tcoord_ptr[out->tcoord_size][1] = 1.0f - y;
                } else {
                    out->tcoord_ptr[out->tcoord_size][1] = y;
                }
                out->tcoord_size++;
            }

            if (str_starts_with(line_buffer, "f ")) {
                // Face
                int x, y, z, tx, ty, tz;
                if (has_uv) {
                    sscanf(line_buffer, "f %d/%d %d/%d %d/%d", &x, &tx, &y, &ty, &z, &tz);
                    out->vface_ptr[out->face_size][0] = x - 1;
                    out->vface_ptr[out->face_size][1] = y - 1;
                    out->vface_ptr[out->face_size][2] = z - 1;
                    out->tface_ptr[out->face_size][0] = tx - 1;
                    out->tface_ptr[out->face_size][1] = ty - 1;
                    out->tface_ptr[out->face_size][2] = tz - 1;
                } else {
                    int x, y, z;
                    sscanf(line_buffer, "f %d %d %d", &x, &y, &z);
                    out->vface_ptr[out->face_size][0] = x - 1;
                    out->vface_ptr[out->face_size][1] = y - 1;
                    out->vface_ptr[out->face_size][2] = z - 1;
                }

                if (has_mat) {
                    out->mface_ptr[out->face_size] = active_mat;
                }

                out->face_size++;
            }

            if (str_starts_with(line_buffer, "usemtl ")) {
                // Materials
                sscanf(line_buffer, "usemtl %s", mat_name);
                active_mat = get_mat_enum(mat_name, mat_array, mat_cnt);
            }

            // Reset line
            char_index = 0;
        } else {
            char_index++;
        }
    }

    minigl_fclose(f);

    return 0;
}

void minigl_obj_free(minigl_obj_t *obj) {
    minigl_safe_free(obj->vcoord_ptr);
    minigl_safe_free(obj->tcoord_ptr);
    minigl_safe_free(obj->vface_ptr);
    minigl_safe_free(obj->tface_ptr);
    minigl_safe_free(obj->mface_ptr);
}

void minigl_obj_copy(minigl_obj_t *in, minigl_obj_t *out) {
    // Copy all fields from in obj
    *out = *in;

    // Allocate
    out->vcoord_ptr = (vec4 *)malloc(in->vcoord_size * sizeof(vec4));

    memcpy(out->vcoord_ptr, in->vcoord_ptr, in->vcoord_size * sizeof(vec4));
}

void minigl_obj_copy_trans(minigl_obj_t *in, mat4 trans, minigl_obj_t *out) {
    // Copy all fields from in obj
    *out = *in;

    // Allocate
    out->vcoord_ptr = (vec4 *)malloc(in->vcoord_size * sizeof(vec4));

    // Apply transformation
    for (int i = 0; i < in->vcoord_size; i++) {
        glm_mat4_mulv(trans, in->vcoord_ptr[i], out->vcoord_ptr[i]);
    }
}

void minigl_obj_trans(minigl_obj_t *in, mat4 trans) {
    // Apply transformation
    for (int i = 0; i < in->vcoord_size; i++) {
        glm_mat4_mulv(trans, in->vcoord_ptr[i], in->vcoord_ptr[i]);
    }
}

minigl_objbuf_t *minigl_objbuf_new(size_t size) {
    minigl_objbuf_t *out = malloc(sizeof(minigl_objbuf_t));
    out->vcoord_ptr = (vec4 *)malloc(size * sizeof(vec4));
    return out;
}

void minigl_objbuf_free(minigl_objbuf_t *buf) {
    free(buf->vcoord_ptr);
    free(buf);
}

void minigl_obj_to_objbuf_trans(minigl_obj_t *in, mat4 trans, minigl_objbuf_t *out) {
    out->tcoord_ptr = in->tcoord_ptr;
    out->vface_ptr = in->vface_ptr;
    out->tface_ptr = in->tface_ptr;
    out->mface_ptr = in->mface_ptr;
    out->vcoord_size = in->vcoord_size;
    out->tcoord_size = in->tcoord_size;
    out->face_size = in->face_size;

    // Apply transformation
    for (int i = 0; i < in->vcoord_size; i++) {
        glm_mat4_mulv(trans, in->vcoord_ptr[i], out->vcoord_ptr[i]);
    }
}
