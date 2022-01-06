#ifndef BUFFER_DEFS_H
#define BUFFER_DEFS_H

typedef GLubyte face_mesh_component_t;
typedef uint32_t buffer_size_t;

typedef GLfloat bb_pos_component_t; // bb = billboard
typedef GLuint bb_texture_id_t; // Max = 4294967295
/* There is no type face_texture_id_t b/c the face texture id is
packed into an info byte, and there's no type to represent five bits */

#define MESH_COMPONENT_TYPENAME GL_UNSIGNED_BYTE
#define BB_POS_COMPONENT_TYPENAME GL_FLOAT
#define BB_TEXTURE_ID_TYPENAME GL_UNSIGNED_INT

enum {
	components_per_face_vertex = 4,
	vertices_per_face = 6,
	components_per_face = components_per_face_vertex * vertices_per_face,
	bytes_per_face_vertex = sizeof(face_mesh_component_t[components_per_face_vertex]),
	bytes_per_face = sizeof(face_mesh_component_t[components_per_face])
};

#endif
