#ifndef SKYBOX_H
#define SKYBOX_H

#include "utils.h"
#include "camera.h"
#include "texture.h"
#include "../data/shaders.c"

typedef struct {
	GLuint vbo, shader, texture;
} Skybox;

// Excluded: init_skybox_texture

Skybox init_skybox(const GLchar* const cubemap_path);
void deinit_skybox(const Skybox s);
void draw_skybox(const Skybox s, const Camera* const camera);

#endif
