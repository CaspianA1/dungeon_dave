#ifndef SKYBOX_H
#define SKYBOX_H

#include "drawable.h"
#include "camera.h"

typedef Drawable Skybox;

#define deinit_skybox deinit_drawable

// Excluded: init_skybox_texture, define_vertex_spec_for_skybox, update_skybox_uniforms

Skybox init_skybox(const GLchar* const cubemap_path, const GLfloat texture_rescale_factor);
void draw_skybox(const Skybox* const skybox, const mat4 model_view_projection);

#endif
