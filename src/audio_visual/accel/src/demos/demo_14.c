#include "demo_12.c"

// later, to culling.c

typedef struct {
	struct {const float near, far;} clip_dists;
	// Technically, the camera stays at {0, 0, 0}, but having the `pos` member is more practical
	GLfloat pos[3], dir[3], right[3], up[3], fov, hori_angle, vert_angle, aspect_ratio;
} Camera;

// A move_camera fn would be nice

// Same batch for different uses, so can re-write sizes and type enum
typedef struct {
	int curr_num_drawable_objects, max_drawable_objects;
	size_t bytes_per_mesh_type, bytes_per_vertex_component;
	GLenum type_enum;
	GLuint vbo;
} Batch;
// In drawing, draw non-height-zero sectors and height-zero sectors in their own calls

// size param can be different for height-zero flat planes and normal sectors
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

// Later on, this may offer the caller different matrices as well, if needed
void get_matrices_from_camera(const Camera* const camera, mat4 model_view_projection) {
	float* const pos = (float*) camera -> pos;
	mat4 projection, view, model_view;

	vec3 rel_origin;
	glm_vec3_add(pos, (float*) camera -> dir, rel_origin);

	glm_perspective(camera -> fov, camera -> aspect_ratio, camera -> clip_dists.near, camera -> clip_dists.far, projection);
	glm_lookat(pos, rel_origin, (float*) camera -> up, view);

	glm_mul(view, (mat4) GLM_MAT4_IDENTITY_INIT, model_view);
	glm_mul(projection, model_view, model_view_projection);
}

void add_unculled_sectors_to_batch(const Camera* const camera, Batch* const batch, const SectorList sector_list) {
	start_batch_adding(batch);

	mat4 model_view_projection;
	get_matrices_from_camera(camera, model_view_projection);

	vec4 frustum_planes[6];
	glm_frustum_planes(model_view_projection, frustum_planes);

	for (int i = 0; i < sector_list.length; i++) {
		const Sector sector = sector_list.data[i];
		const SectorArea area = sector.area;

		// In format of bottom left and top right
		vec3 aabb_corners[2] = {{area.origin[0], 0.0f, area.origin[1]}};
		glm_vec3_add(aabb_corners[0], (vec3) {area.size[0], area.height, area.size[1]}, aabb_corners[1]);

		// If sector in view
		if (glm_aabb_frustum(aabb_corners, frustum_planes)) {
			printf("Draw #%d\n", i + 1);
			add_to_batch(batch, NULL); // Feed sector vertices later on; store in sector, instead of vbo
		}
	}
}

#ifdef DEMO_14
int main(void) {
}
#endif
