#include <cglm/cglm.h>
#include <libgen.h>
#include <string.h>

#include "minigl/frame.h"
#include "minigl/minigl.h"
#include "minigl/texture.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#define SCREEN_SIZE_X 800
#define SCREEN_SIZE_Y 600

minigl_objbuf_t* buf;
mat4 trans;
mat4 view;
mat4 proj;

typedef enum {
    MINIGL_MATERIAL_COLOR,
    MINIGL_MATERIAL_TEXTURE
} minigl_material_type_t;

typedef struct {
    minigl_material_type_t type;
    uint8_t color;
    minigl_tex_t* tex;
} minigl_material_t;

minigl_material_t MAT_UNKNOWN = {.type = MINIGL_MATERIAL_COLOR, .color = 255};

typedef struct {
    minigl_obj_t* object;
    minigl_material_t* material;
} minigl_item_t;

typedef struct {
    char base_path[1024];
    cgltf_data* data;

    minigl_tex_t** gltf_images;
    minigl_material_t** gltf_materials;
    minigl_item_t** gltf_items;
    int gltf_items_cnt;
} gltf_context_t;

void gltf_parse_images(gltf_context_t* context) {
    context->gltf_images = malloc(context->data->images_count * sizeof(minigl_tex_t*));
    for (cgltf_size i = 0; i < context->data->images_count; i++) {
        cgltf_image* image = &context->data->images[i];

        char image_path[1024];
        snprintf(image_path, sizeof(image_path), "%s/%s", context->base_path, image->uri);

        printf("Reading image %s\n", image_path);
        minigl_tex_read_png(image_path, MINIGL_TEX_READ_OPTS_NONE);
        minigl_tex_t* tex = minigl_tex_read_png(image_path, MINIGL_TEX_READ_OPTS_NONE);
        if (tex == NULL) {
            printf("Can't find image %zu: %s\n", i, image_path);
        }

        context->gltf_images[i] = tex;
    }
}

void gltf_parse_materials(gltf_context_t* context) {
    context->gltf_materials = malloc(context->data->materials_count * sizeof(minigl_material_t*));
    for (cgltf_size i = 0; i < context->data->materials_count; i++) {
        context->gltf_materials[i] = malloc(sizeof(minigl_material_t));

        cgltf_material* material = &context->data->materials[i];
        printf("Material %zu: %s\n", i, material->name ? material->name : "Unnamed");

        if (material->has_pbr_metallic_roughness) {
            if (material->pbr_metallic_roughness.base_color_texture.texture != NULL) {
                cgltf_texture* texture = material->pbr_metallic_roughness.base_color_texture.texture;
                context->gltf_materials[i]->type = MINIGL_MATERIAL_TEXTURE;
                context->gltf_materials[i]->tex = context->gltf_images[cgltf_image_index(context->data, texture->image)];
            }
        }
    }
}

void gltf_parse_primitive_triangles(gltf_context_t* context, cgltf_primitive* primitive, minigl_item_t* item) {
    // Allocate object
    minigl_obj_t* obj = malloc(sizeof(minigl_obj_t));
    item->object = obj;

    if (primitive->material != NULL) {
        item->material = context->gltf_materials[cgltf_material_index(context->data, primitive->material)];
    } else {
        item->material = &MAT_UNKNOWN;
    }

    // Extract indices
    cgltf_accessor* indices = primitive->indices;

    obj->face_size = indices->count / 3;
    obj->tcoord_size = indices->count / 3;

    obj->vface_ptr = malloc(sizeof(ivec3) * obj->face_size);
    obj->tface_ptr = malloc(sizeof(ivec3) * obj->face_size);
    obj->mface_ptr = NULL;

    uint16_t* indices_array = malloc(indices->count * sizeof(uint16_t));
    cgltf_accessor_unpack_indices(indices, indices_array, sizeof(uint16_t), indices->count);

    for (int i = 0; i < obj->face_size; i++) {
        // Face indices
        obj->vface_ptr[i][0] = indices_array[i * 3 + 0];
        obj->vface_ptr[i][1] = indices_array[i * 3 + 1];
        obj->vface_ptr[i][2] = indices_array[i * 3 + 2];

        // Texture face indices
        obj->tface_ptr[i][0] = indices_array[i * 3 + 0];
        obj->tface_ptr[i][1] = indices_array[i * 3 + 1];
        obj->tface_ptr[i][2] = indices_array[i * 3 + 2];
    }
    free(indices_array);

    // Extract attributes
    for (cgltf_size i = 0; i < primitive->attributes_count; i++) {
        cgltf_attribute* attr = &primitive->attributes[i];
        cgltf_accessor* accessor = attr->data;

        if (strcmp(attr->name, "POSITION") == 0) {
            // Extract vertex coordinates
            obj->vcoord_size = accessor->count;
            obj->vcoord_ptr = malloc(sizeof(vec4) * obj->vcoord_size);

            float* values = malloc(accessor->count * 3 * sizeof(float));
            cgltf_accessor_unpack_floats(accessor, values, accessor->count * 3);
            for (cgltf_size j = 0; j < accessor->count; j++) {
                obj->vcoord_ptr[j][0] = values[j * 3 + 0];
                obj->vcoord_ptr[j][1] = values[j * 3 + 1];
                obj->vcoord_ptr[j][2] = values[j * 3 + 2];
                obj->vcoord_ptr[j][3] = 1.0f;  // Homogeneous coordinate
            }
            free(values);
        } else if (strcmp(attr->name, "TEXCOORD_0") == 0) {
            // Extract texture coordinates
            obj->tcoord_ptr = malloc(sizeof(vec4) * accessor->count);

            float* values = malloc(accessor->count * 2 * sizeof(float));
            cgltf_accessor_unpack_floats(accessor, values, accessor->count * 2);
            for (cgltf_size j = 0; j < accessor->count; j++) {
                obj->tcoord_ptr[j][0] = values[j * 2 + 0];
                obj->tcoord_ptr[j][1] = values[j * 2 + 1];
            }
            free(values);
        } else if (strcmp(attr->name, "NORMAL") == 0) {
            // TODO
        } else {
            assert(0 && "Unsupported attribute type");
        }
    }
}

void nodetrans_to_mat4(cgltf_node* node, mat4 dst) {
    cgltf_float matrix[16];
    cgltf_node_transform_world(node, matrix);
    for (int k = 0; k < 16; k++) {
        dst[k / 4][k % 4] = matrix[k];
    }
}

void gltf_parse_nodes(gltf_context_t* context) {
    context->gltf_items_cnt = 0;
    context->gltf_items = NULL;

    for (cgltf_size i = 0; i < context->data->nodes_count; i++) {
        cgltf_node* node = &context->data->nodes[i];
        printf("Node %zu: %s\n", i, node->name ? node->name : "Unnamed");
        if (node->mesh) {
            cgltf_mesh* mesh = node->mesh;
            printf("  Mesh: %s\n", mesh->name ? mesh->name : "Unnamed");
            for (cgltf_size j = 0; j < mesh->primitives_count; j++) {
                cgltf_primitive* primitive = &mesh->primitives[j];
                if (primitive->type == cgltf_primitive_type_triangles) {
                    context->gltf_items = realloc(context->gltf_items, sizeof(minigl_item_t*) * (context->gltf_items_cnt + 1));
                    context->gltf_items[context->gltf_items_cnt] = malloc(sizeof(minigl_item_t));
                    gltf_parse_primitive_triangles(context, primitive, context->gltf_items[context->gltf_items_cnt]);

                    mat4 model;
                    nodetrans_to_mat4(node, model);
                    minigl_obj_trans(context->gltf_items[context->gltf_items_cnt]->object, model);

                    context->gltf_items_cnt++;
                } else {
                    assert(0 && "Unsupported primitive type, only triangles are supported");
                }
            }
        }

        // Camera
        if (node->camera) {
            cgltf_camera* camera = node->camera;
            printf("  Camera: %s\n", camera->name ? camera->name : "Unnamed");

            if (camera->type == cgltf_camera_type_perspective) {
                cgltf_camera_perspective perspective = camera->data.perspective;

                // View matrix
                nodetrans_to_mat4(node, view);
                glm_mat4_inv(view, view);

                // Projection matrix
                float zfar = 100.0f;
                if (perspective.has_zfar) {
                    zfar = perspective.zfar;
                }

                float aspect_ratio = (float)SCREEN_SIZE_X / (float)SCREEN_SIZE_Y;
                if (perspective.has_aspect_ratio) {
                    aspect_ratio = perspective.aspect_ratio;
                }
                glm_perspective(perspective.yfov, aspect_ratio, perspective.znear, zfar, proj);
            } else if (camera->type == cgltf_camera_type_orthographic) {
                assert(0 && "Orthographic cameras are not supported yet.");
            }
        }
    }
}

gltf_context_t* gltf_load(const char* path) {
    gltf_context_t* context = malloc(sizeof(gltf_context_t));
    if (!context) {
        printf("Failed to allocate memory for glTF context.\n");
        return NULL;
    }

    printf("Reading %s\n", path);

    // Extract the base dir
    strncpy(context->base_path, path, sizeof(context->base_path));
    dirname(context->base_path);

    // Load data
    cgltf_options options = {0};
    cgltf_result result = cgltf_parse_file(&options, path, &context->data);

    if (result != cgltf_result_success) {
        printf("Failed to load glTF file: %s\n", path);
        return NULL;
    }
    result = cgltf_load_buffers(&options, context->data, path);
    if (result != cgltf_result_success) {
        printf("Failed to load glTF buffers.\n");
        cgltf_free(context->data);
        return NULL;
    }

    gltf_parse_images(context);
    gltf_parse_materials(context);
    gltf_parse_nodes(context);

    return context;
}

void gltf_context_free(gltf_context_t* context) {
    if (!context) return;

    // Free images
    for (cgltf_size i = 0; i < context->data->images_count; i++) {
        if (context->gltf_images[i]) {
            minigl_tex_free(context->gltf_images[i]);
        }
    }
    free(context->gltf_images);

    // Free materials
    for (cgltf_size i = 0; i < context->data->materials_count; i++) {
        free(context->gltf_materials[i]);
    }
    free(context->gltf_materials);

    // Free items
    for (int i = 0; i < context->gltf_items_cnt; i++) {
        minigl_obj_free(context->gltf_items[i]->object);
        free(context->gltf_items[i]);
    }
    free(context->gltf_items);

    cgltf_free(context->data);
    free(context);
}

void gltf_render_scene(gltf_context_t* context) {
    for (int i = 0; i < context->gltf_items_cnt; i++) {
        minigl_item_t* item = context->gltf_items[i];

        minigl_obj_to_objbuf_trans(item->object, trans, buf);

        minigl_set_color(255);

        switch (item->material->type) {
            case MINIGL_MATERIAL_COLOR:
                minigl_set_color(item->material->color);
                break;
            case MINIGL_MATERIAL_TEXTURE:
                minigl_set_tex(*item->material->tex);
                break;
        }

        minigl_draw(buf);
    }
}

void minigl_perf_print(void) {
    minigl_perf_data_t perf_data = minigl_perf_get();
#ifdef MINIGL_DEBUG_PERF
    printf("Clip count: %d\n", perf_data.clip);
    printf("Cull count: %d\n", perf_data.cull);
    printf("Poly count: %d\n", perf_data.poly);
    printf("Frag count: %d\n", perf_data.frag);
#endif
    minigl_perf_clear();
}

int main(int argc, char** argv) {
    // Parse arguments
    if (argc != 2) {
        return 1;
    }

    //---------------------------------------------------------------------------
    // MiniGL
    //---------------------------------------------------------------------------
    minigl_frame_t* frame = minigl_frame_new(SCREEN_SIZE_X, SCREEN_SIZE_Y);
    minigl_set_frame(frame);
    buf = minigl_objbuf_new(100000);
    // minigl_tex_t* dither = minigl_tex_read_png(argv[1], MINIGL_TEX_READ_OPTS_NONE);
    // minigl_set_dither(*dither);

    //---------------------------------------------------------------------------
    // GLTF
    //---------------------------------------------------------------------------

    // Load glTF scene
    const char* gltf_file_path = argv[1];
    gltf_context_t* context;
    context = gltf_load(gltf_file_path);
    if (context == NULL) {
        return 1;
    }

    //---------------------------------------------------------------------------
    // Render
    //---------------------------------------------------------------------------

    glm_mat4_mul(proj, view, trans);

    // Clear the frame
    minigl_clear(0.0f, 1.0f);

    // Render the scene
    gltf_render_scene(context);

    // Save frame
    minigl_frame_to_file(frame, "out.png");

    // Free memory
    gltf_context_free(context);

    return 0;
}
