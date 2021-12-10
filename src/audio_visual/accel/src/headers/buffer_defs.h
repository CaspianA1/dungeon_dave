#ifndef BUFFER_DEFS_H
#define BUFFER_DEFS_H

typedef GLubyte mesh_component_t;
typedef GLuint buffer_index_t;

typedef GLfloat bb_pos_component_t; // bb = billboard
typedef GLushort bb_texture_id_t; // Max = 65536
/* There is no face_texture_id_t b/c the face texture id is packed into
an info byte, and there's no type to represent five bits */

#define MESH_COMPONENT_TYPENAME GL_UNSIGNED_BYTE
#define BUFFER_INDEX_TYPENAME GL_UNSIGNED_INT

enum {
	vars_per_vertex = 4,
	vertices_per_face = 4,
	indices_per_face = 6,

	bytes_per_vertex = vars_per_vertex * sizeof(mesh_component_t),
	vars_per_face = vars_per_vertex * vertices_per_face
};

#endif
