#ifndef SKYBOX_H
#define SKYBOX_H

#include "utils.h"
#include "camera.h"

typedef struct {
	GLuint vbo, shader, texture;
} Skybox;

// Excluded: init_skybox_texture

Skybox init_skybox(const char* const cubemap_path);
void deinit_skybox(const Skybox s);
void draw_skybox(const Skybox s, const Camera* const camera);

#endif