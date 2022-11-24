#ifndef BUFFER_DEFS_H
#define BUFFER_DEFS_H

#include "lib/glad/glad.h" // For OpenGL defs

//////////

// TODO: make a `texture_size_t` type

typedef signed char signed_byte;
typedef GLubyte byte;
typedef GLuint buffer_size_t; // Max = 4294967295

/*Note: the texture id typename does not apply to sectors'
texture IDs, since those are bytes. */
typedef GLushort material_index_t;
typedef GLushort texture_id_t; // Max = 65535
typedef GLushort billboard_index_t; 
typedef GLubyte face_component_t;

//////////

#define MATERIAL_INDEX_TYPENAME GL_UNSIGNED_SHORT
#define TEXTURE_ID_TYPENAME GL_UNSIGNED_SHORT
#define BILLBOARD_INDEX_TYPENAME GL_UNSIGNED_SHORT
#define FACE_COMPONENT_TYPENAME GL_UNSIGNED_BYTE

#endif
