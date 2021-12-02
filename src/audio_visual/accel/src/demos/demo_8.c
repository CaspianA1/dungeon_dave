#include "demo_7.c"

/*
- optimized plane drawing
- want a solid base before continuing
- check for negative vals with plane data in create_vert_plane_interleaved
- in demo 9, allow for 2 interleaved planes to be drawn in the same pass
*/

#define PLANE_TYPE GLfloat
#define PLANE_TYPE_ENUM GL_FLOAT

/*
#define PLANE_TYPE GLushort
#define PLANE_TYPE_ENUM GL_UNSIGNED_SHORT
*/

enum {interleaved_plane_vars = 20};
enum {interleaved_plane_bytes = interleaved_plane_vars * sizeof(PLANE_TYPE)};

void create_vert_plane_interleaved_1(const PLANE_TYPE* const top_left_corner,
	const PLANE_TYPE width, const PLANE_TYPE height, PLANE_TYPE* const plane_buffer) {

	const PLANE_TYPE left_x = top_left_corner[0], top_y = top_left_corner[1], z = top_left_corner[2];
	const PLANE_TYPE right_x = left_x + width, bottom_y = top_y - height;

	const PLANE_TYPE vertices_with_UV[interleaved_plane_vars] = {
		left_x, top_y, z, 0.0f, 0.0f, // last two floats are UV
		right_x, top_y, z, width, 0.0f,

		left_x, bottom_y, z, 0.0f, height,
		right_x, bottom_y, z, width, height
	};

	memcpy(plane_buffer, vertices_with_UV, interleaved_plane_bytes);
}

void create_vert_plane_interleaved_2(const PLANE_TYPE* const top_left_corner,
	const PLANE_TYPE width, const PLANE_TYPE height, PLANE_TYPE* const plane_buffer) {

	const PLANE_TYPE x = top_left_corner[0], top_y = top_left_corner[1], far_z = top_left_corner[2];
	const PLANE_TYPE bottom_y = top_y - height, near_z = far_z + width;

	const PLANE_TYPE vertices_with_UV[interleaved_plane_vars] = {
		x, top_y, far_z, 0.0f, 0.0f,
		x, top_y, near_z, width, 0.0f,

		x, bottom_y, far_z, 0.0f, height,
		x, bottom_y, near_z, width, height
	};

	memcpy(plane_buffer, vertices_with_UV, interleaved_plane_bytes);
}

void create_hori_plane_interleaved(const PLANE_TYPE* const top_left_corner,
	const PLANE_TYPE width, const PLANE_TYPE height, PLANE_TYPE* const plane_buffer) {

	const PLANE_TYPE left_x = top_left_corner[0], y_height = top_left_corner[1], depth_origin = top_left_corner[2];
	const PLANE_TYPE right_x = left_x + width, largest_depth = depth_origin + height;

	const PLANE_TYPE vertices_with_UV[interleaved_plane_vars] = {
		left_x, y_height, depth_origin, 0.0f, 0.0f,
		right_x, y_height, depth_origin, width, 0.0f,

		left_x, y_height, largest_depth, 0.0f, height,
		right_x, y_height, largest_depth, width, height
	};

	memcpy(plane_buffer, vertices_with_UV, interleaved_plane_bytes);
}

typedef struct {
	const PlaneType type;
	const PLANE_TYPE top_left_corner[3];
	const int width, height;
} PlaneDefInterleaved;

// Plane type, top left corner, size hori, size vert
PLANE_TYPE* create_plane_mesh_interleaved(const int num_planes, ...) {
	va_list args;
	va_start(args, num_planes);
	PLANE_TYPE* const all_vertex_data = malloc(num_planes * interleaved_plane_bytes);

	for (int i = 0; i < num_planes; i++) {
		const PlaneDefInterleaved plane_def = va_arg(args, PlaneDefInterleaved);

		void (*plane_creator)(const PLANE_TYPE* const,
			const PLANE_TYPE, const PLANE_TYPE, PLANE_TYPE* const);

		switch (plane_def.type) {
			case Hori:
				plane_creator = create_hori_plane_interleaved; break;
			case Vert_1:
				plane_creator = create_vert_plane_interleaved_1; break;
			case Vert_2:
				plane_creator = create_vert_plane_interleaved_2; break;
		}
		plane_creator(plane_def.top_left_corner, plane_def.width,
			plane_def.height, all_vertex_data + i * interleaved_plane_vars);
	}

	va_end(args);
	return all_vertex_data;
}

void bind_interleaved_planes_to_vao(void) {
	enum {interleaved_vertex_bytes = 5 * sizeof(PLANE_TYPE)};

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, PLANE_TYPE_ENUM, GL_FALSE, interleaved_vertex_bytes, NULL);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, PLANE_TYPE_ENUM, GL_FALSE, interleaved_vertex_bytes, (void*) (3 * sizeof(PLANE_TYPE)));
}

StateGL demo_8_init(void) {
	StateGL sgl;
	sgl.vertex_array = init_vao();

	const PLANE_TYPE origin[3] = {1, 1, 1};
	enum {size_hori = 8, size_vert = 5, size_hori_2 = 2, size_vert_2 = 3, size_hori_3 = 5, num_planes = 4};

	PLANE_TYPE
		*const plane_data = create_plane_mesh_interleaved(num_planes,
			(PlaneDefInterleaved) {Hori, {origin[0], origin[1], origin[2]}, size_hori_2, size_vert_2},
			(PlaneDefInterleaved) {Vert_1, {origin[0], size_vert + origin[1], origin[2]}, size_hori, size_vert},
			(PlaneDefInterleaved) {Vert_1, {origin[0], origin[1] + size_vert, size_vert + origin[2]}, size_hori, size_vert},
			(PlaneDefInterleaved) {Vert_2, {9.0f, 6.0f, 1.0f}, size_hori_3, size_vert + 1}
		);

	sgl.num_vertex_buffers = 1;
	sgl.vertex_buffers = init_vbos(sgl.num_vertex_buffers, plane_data, interleaved_plane_bytes * num_planes);
	bind_interleaved_planes_to_vao();
	free(plane_data);

	sgl.shader_program = init_shader_program(demo_4_vertex_shader, demo_4_fragment_shader);
	glUseProgram(sgl.shader_program);
	sgl.num_textures = 1;
	sgl.textures = init_textures(sgl.num_textures, "../../../assets/walls/pyramid.bmp", tex_repeating);
	select_texture_for_use(sgl.textures[0], sgl.shader_program);
	enable_all_culling();

	return sgl;
}

void demo_8_drawer(const StateGL* const sgl) {
	move(sgl -> shader_program);
	glClearColor(0.4f, 0.0f, 0.0f, 0.0f); // Dark blue
	const int num_planes = 4;
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4 * num_planes);
}

#ifdef DEMO_8
int main(void) {
	make_application(demo_8_drawer, demo_8_init, deinit_demo_vars);
}
#endif
