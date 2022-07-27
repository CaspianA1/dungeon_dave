#ifndef BUFFER_DEFS_H
#define BUFFER_DEFS_H

#include <stdbool.h>
#include "glad/glad.h"

////////// cglm and SDL2 have plenty of floating-point promotion errors, so this ignores them

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdouble-promotion"

#include "cglm/cglm.h"

// If SDL2/SDL.h isn't found, try SDL.h
#if __has_include("SDL2/SDL.h")
	#include <SDL2/SDL.h>
#else
	#include <SDL.h>
#endif

#pragma clang diagnostic pop

//////////

typedef signed char signed_byte;
typedef GLubyte byte;
typedef GLubyte face_mesh_component_t;
typedef GLuint buffer_size_t; // Max = 4294967295
typedef GLfloat billboard_var_component_t;
typedef GLushort billboard_index_t; // Max = 65535

#define FACE_MESH_COMPONENT_TYPENAME GL_UNSIGNED_BYTE
#define BUFFER_SIZE_TYPENAME GL_UNSIGNED_INT
#define BILLBOARD_VAR_COMPONENT_TYPENAME GL_FLOAT

// TODO: put this in constants.h
enum { // `enum` is used to make these values compile-time constants
	components_per_face_vertex = 4,
	vertices_per_face = 6,

	vertices_per_triangle = 3,
	corners_per_quad = 4,
	corners_per_frustum = 8,
	planes_per_frustum = 6,
	faces_per_cubemap = 6,
	vertices_per_skybox = 14
};

#endif
