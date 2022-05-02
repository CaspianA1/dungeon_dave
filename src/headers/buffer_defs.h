#ifndef BUFFER_DEFS_H
#define BUFFER_DEFS_H

#include <SDL2/SDL.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#include <cglm/cglm.h>
#pragma GCC diagnostic pop

#include <stdbool.h>

#include "../../include/glad/glad.h"

typedef GLubyte byte;
typedef GLubyte face_mesh_component_t;
typedef GLuint buffer_size_t; // Max = 4294967295
typedef GLfloat billboard_var_component_t;

#define FACE_MESH_COMPONENT_TYPENAME GL_UNSIGNED_BYTE
#define BILLBOARD_VAR_COMPONENT_TYPENAME GL_FLOAT
#define BUFFER_SIZE_TYPENAME GL_UNSIGNED_INT

enum { // `enum` is used to make these values compile-time constants
	components_per_face_vertex = 4,
	vertices_per_face = 6,
	components_per_face = components_per_face_vertex * vertices_per_face,
	bytes_per_face_vertex = sizeof(face_mesh_component_t[components_per_face_vertex]),
	bytes_per_face = sizeof(face_mesh_component_t[components_per_face]),

	corners_per_quad = 4
};

#endif
