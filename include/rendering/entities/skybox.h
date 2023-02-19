#ifndef SKYBOX_H
#define SKYBOX_H

#include "rendering/drawable.h" // For `Drawable`

typedef struct {
    const GLchar* const texture_path;
    const GLfloat texture_scale, horizon_dist_scale, y_shift_offset;
    const bool apply_cylindrical_projection;
} SkyboxConfig;

//////////

typedef struct {
    const Drawable drawable;
} Skybox;

// Excluded: init_skybox_texture, define_vertex_spec

Skybox init_skybox(const SkyboxConfig* const config);
void deinit_skybox(const Skybox* const skybox);
void draw_skybox(const Skybox* const skybox);

#endif
