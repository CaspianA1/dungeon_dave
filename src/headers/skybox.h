#ifndef SKYBOX_H
#define SKYBOX_H

#include "utils.h"
#include "camera.h"
#include "texture.h"

typedef struct {
	GLuint vbo, shader, texture;
} Skybox;

// Excluded: init_skybox_texture

Skybox init_skybox(const GLchar* const cubemap_path, const GLfloat texture_rescale_factor);
void deinit_skybox(const Skybox s);
void draw_skybox(const Skybox s, const Camera* const camera);

#endif
