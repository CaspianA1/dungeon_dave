// A 'batch' is a collection of objects to be rendered in one draw call.

/* Same batch for different uses, so can re-write sizes and type enum
In drawing, draw non-height-zero sectors and height-zero sectors in their own calls */

typedef struct {
	int curr_num_drawable_objects, max_drawable_objects;
	size_t bytes_per_mesh_type, bytes_per_vertex_component;
	GLenum type_enum;
	GLuint vbo;
} Batch;

// bytes_per_mesh_type param can be different for height-zero flat planes and normal sectors
Batch init_batch(const int max_drawable_objects, const size_t bytes_per_mesh_type,
	const size_t bytes_per_vertex_component, const GLenum type_enum) {

	Batch batch = {
		.max_drawable_objects = max_drawable_objects,
		.bytes_per_mesh_type = bytes_per_mesh_type,
		.bytes_per_vertex_component = bytes_per_vertex_component,
		.type_enum = type_enum
	};

	glGenBuffers(1, &batch.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, batch.vbo);
	glBufferData(GL_ARRAY_BUFFER, max_drawable_objects * bytes_per_mesh_type, NULL, GL_DYNAMIC_DRAW);
	return batch;
}

void deinit_batch(const Batch* const batch) {
	glDeleteBuffers(1, &batch -> vbo);
}

void start_batch_adding(Batch* const batch) {
	batch -> curr_num_drawable_objects = 0;
	glBindBuffer(GL_ARRAY_BUFFER, batch -> vbo);

	const GLenum type_enum = batch -> type_enum;
	const size_t bytes_per_vertex_component = batch -> bytes_per_vertex_component;

	const size_t
		bytes_per_vertex = 5 * bytes_per_vertex_component,
		bytes_per_vertex_component_pos = 3 * bytes_per_vertex_component;
	
	/*
	- 5 components: x, y, z, u, v.
	- For sectors, all components are bytes; otherwise, all components are floats.
	*/

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, type_enum, GL_FALSE, bytes_per_vertex, NULL);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, type_enum, GL_FALSE, bytes_per_vertex, (void*) bytes_per_vertex_component_pos);
}

// Works assuming that sector batch vbo is already bound
void add_to_batch(Batch* const batch, const void* const data) {
	const size_t mesh_type_bytes = batch -> bytes_per_mesh_type;
	const GLsizeiptr batch_entry_offset = batch -> curr_num_drawable_objects++ * mesh_type_bytes;
	glBufferSubData(GL_ARRAY_BUFFER, batch_entry_offset, mesh_type_bytes, data);
}