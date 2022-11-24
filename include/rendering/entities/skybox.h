#ifndef SKYBOX_H
#define SKYBOX_H

#include "rendering/drawable.h" // For `Drawable`

typedef Drawable Skybox;

// Excluded: init_skybox_texture

#define deinit_skybox deinit_drawable

Skybox init_skybox(const GLchar* const texture_path);
void draw_skybox(const Skybox* const skybox);

#endif
