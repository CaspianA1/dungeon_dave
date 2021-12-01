#ifndef BUFFER_DEFS_H
#define BUFFER_DEFS_H

typedef GLubyte mesh_type_t;
typedef GLuint index_type_t;

#define MESH_TYPE_ENUM GL_UNSIGNED_BYTE
#define INDEX_TYPE_ENUM GL_UNSIGNED_INT

enum {
	vars_per_vertex = 4,
	vertices_per_face = 4,
	indices_per_face = 6,

	bytes_per_vertex = vars_per_vertex * sizeof(mesh_type_t),
	vars_per_face = vars_per_vertex * vertices_per_face
};

#endif
