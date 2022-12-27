#ifndef SKYBOX_H
#define SKYBOX_H

#include "rendering/drawable.h" // For `Drawable`

typedef struct {
    const GLchar* const texture_path;
    const bool map_cube_to_sphere;
} SkyboxConfig;

//////////

typedef Drawable Skybox;

// Excluded: init_skybox_texture

#define deinit_skybox deinit_drawable

Skybox init_skybox(const SkyboxConfig* const config);
void draw_skybox(const Skybox* const skybox);

#endif
