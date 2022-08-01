#ifndef SKYBOX_H
#define SKYBOX_H

#include "rendering/drawable.h"
#include "camera.h"

typedef Drawable Skybox;

#define deinit_skybox deinit_drawable

// Excluded: init_skybox_texture, update_uniforms

Skybox init_skybox(const GLchar* const cubemap_path);
void draw_skybox(const Skybox* const skybox);

#endif
