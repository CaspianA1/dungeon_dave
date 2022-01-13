#ifndef BUFFER_DEFS_H
#define BUFFER_DEFS_H

typedef GLubyte face_mesh_component_t;
typedef GLuint buffer_size_t; // Max = 4294967295

typedef GLfloat bb_pos_component_t; // bb = billboard

#define MESH_COMPONENT_TYPENAME GL_UNSIGNED_BYTE
#define BB_POS_COMPONENT_TYPENAME GL_FLOAT
#define BUFFER_SIZE_TYPENAME GL_UNSIGNED_INT

enum {
	components_per_face_vertex = 4,
	vertices_per_face = 6,
	components_per_face = components_per_face_vertex * vertices_per_face,
	bytes_per_face_vertex = sizeof(face_mesh_component_t[components_per_face_vertex]),
	bytes_per_face = sizeof(face_mesh_component_t[components_per_face])
};

#endif
