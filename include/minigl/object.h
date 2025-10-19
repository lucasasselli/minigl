#pragma once

#include <cglm/cglm.h>

typedef struct {
    int size;
    uint8_t color[];
} minigl_matgroup_t;

struct minigl_obj {
    vec4* vcoord_ptr;
    vec2* tcoord_ptr;
    ivec3* vface_ptr;
    ivec3* tface_ptr;
    int* mface_ptr;
    int vcoord_size;
    int tcoord_size;
    int face_size;
};

typedef struct minigl_obj minigl_obj_t;

// NOTE: This is identical to minigl_obj_t, but used to buffer geometry
// Potentially can be used interchangeably.
typedef struct minigl_obj minigl_objbuf_t;

enum {
    MINIGL_OBJ_TEXFLIPY = 1,
};

int minigl_obj_read_file(char* path, minigl_obj_t* out, int flags);

void minigl_obj_free(minigl_obj_t* obj);

void minigl_obj_copy(minigl_obj_t* in, minigl_obj_t* out);

void minigl_obj_copy_trans(minigl_obj_t* in, mat4 trans, minigl_obj_t* out);

void minigl_obj_trans(minigl_obj_t* in, mat4 trans);

minigl_objbuf_t* minigl_objbuf_new(size_t size);

void minigl_objbuf_free(minigl_objbuf_t* buf);

void minigl_obj_to_objbuf_trans(minigl_obj_t* in, mat4 trans, minigl_objbuf_t* out);
