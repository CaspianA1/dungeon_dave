#include "demo_5.c"

// Demo 10, but just with triangles in correct winding order

typedef GLubyte plane_type_t;
#define PLANE_TYPE_ENUM GL_UNSIGNED_BYTE

enum {
	vars_per_vertex = 5, vertices_per_triangle = 3,
	// Bottom flat triangles of mesh excluded since they will never be seen (so not 12, but 10)
	triangles_per_mesh = 10, triangles_per_height_zero_mesh = 2
};

enum {
	bytes_per_vertex = vars_per_vertex * sizeof(plane_type_t),
	vars_per_triangle = vars_per_vertex * vertices_per_triangle
};

enum {
	vars_per_mesh = vars_per_triangle * triangles_per_mesh,
	vars_per_height_zero_mesh = vars_per_triangle * triangles_per_height_zero_mesh
};

enum {
	bytes_per_mesh = vars_per_mesh * sizeof(plane_type_t),
	bytes_per_height_zero_mesh = vars_per_height_zero_mesh * sizeof(plane_type_t)
};

void check_for_mesh_out_of_bounds(const plane_type_t origin[3], const plane_type_t size[3]) {
	for (byte i = 0; i < 3; i++) {
		const GLint start = origin[i], length = size[i];
		const GLint end = start + ((i == 1) ? -length : length);

		if (start < 0 || start > 255 || end < 0 || end > 255) {
			fprintf(stderr, "Mesh out of bounds on %c axis\n", 'x' + i);
			fail("create mesh: mesh out of bounds", MeshOutOfBounds);
		}
	}
}

/* Even if normal sector meshes can represent height zero meshes, it's worth it to make a separate type
mesh for when a sector has height 0 because I save 120 bytes of memory that way (480 with floats!) */
void create_height_zero_mesh(const plane_type_t origin[3], const plane_type_t size[2], plane_type_t* const dest) {
	check_for_mesh_out_of_bounds(origin, (plane_type_t[3]) {size[0], 0, size[1]});

	const plane_type_t
		size_x = size[0], size_z = size[1],
		near_x = origin[0], top_y = origin[1], near_z = origin[2];

	const plane_type_t far_x = near_x + size_x, far_z = near_z + size_z;

	const plane_type_t vertices[vars_per_height_zero_mesh] = {
		near_x, top_y, far_z, size_z, size_x,
		far_x, top_y, near_z, 0, 0,
		near_x, top_y, near_z, 0, size_x,

		near_x, top_y, far_z, size_z, size_x,
		far_x, top_y, far_z, size_z, 0,
		far_x, top_y, near_z, 0, 0
	};

	memcpy(dest, vertices, bytes_per_height_zero_mesh);
}

void create_sector_mesh(const plane_type_t origin[3], const plane_type_t size[3], plane_type_t* const dest) {
	check_for_mesh_out_of_bounds(origin, size);

	const plane_type_t
		near_x = origin[0], top_y = origin[1], near_z = origin[2],
		size_x = size[0], size_y = size[1], size_z = size[2];

	const plane_type_t far_x = near_x + size_x, bottom_y = top_y - size_y, far_z = near_z + size_z;

	// Side descriptions assume that camera direction is aligned to X axis
	const plane_type_t vertices[vars_per_mesh] = {
		// Face 1, pointing towards Z, near X, front face
		near_x, bottom_y, near_z, 0, size_y,
		near_x, top_y, far_z, size_z, 0,
		near_x, top_y, near_z, 0, 0,

		near_x, bottom_y, near_z, 0, size_y,
		near_x, bottom_y, far_z, size_z, size_y,
		near_x, top_y, far_z, size_z, 0,

		// Face 2, pointing towards Z, far X, back face
		far_x, top_y, near_z, size_z, 0,
		far_x, top_y, far_z, 0, 0,
		far_x, bottom_y, near_z, size_z, size_y,

		far_x, top_y, far_z, 0, 0,
		far_x, bottom_y, far_z, 0, size_y,
		far_x, bottom_y, near_z, size_z, size_y,

		// Face 3, pointing towards X, left face
		near_x, top_y, near_z, size_x, 0,
		far_x, top_y, near_z, 0, 0,
		near_x, bottom_y, near_z, size_x, size_y,

		far_x, top_y, near_z, 0, 0,
		far_x, bottom_y, near_z, 0, size_y,
		near_x, bottom_y, near_z, size_x, size_y,

		// Face 4, pointing towards X, right face
		near_x, bottom_y, far_z, 0, size_y,
		far_x, top_y, far_z, size_x, 0,
		near_x, top_y, far_z, 0, 0,

		near_x, bottom_y, far_z, 0, size_y,
		far_x, bottom_y, far_z, size_x, size_y,
		far_x, top_y, far_z, size_x, 0,

		// Face 5, top flat face
		near_x, top_y, far_z, size_z, size_x,
		far_x, top_y, near_z, 0, 0,
		near_x, top_y, near_z, 0, size_x,

		near_x, top_y, far_z, size_z, size_x,
		far_x, top_y, far_z, size_z, 0,
		far_x, top_y, near_z, 0, 0
	};

	memcpy(dest, vertices, bytes_per_mesh);
}

void bind_interleaved_planes_to_vao(void) {
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	/*
	glVertexAttribIPointer(0, 3, PLANE_TYPE_ENUM, bytes_per_vertex, NULL);
	glVertexAttribIPointer(1, 2, PLANE_TYPE_ENUM, bytes_per_vertex, (void*) (3 * sizeof(plane_type_t)));
	*/

	glVertexAttribPointer(0, 3, PLANE_TYPE_ENUM, GL_FALSE, bytes_per_vertex, NULL);
	glVertexAttribPointer(1, 2, PLANE_TYPE_ENUM, GL_FALSE, bytes_per_vertex, (void*) (3 * sizeof(plane_type_t)));
}

StateGL demo_11_init(void) {
	StateGL sgl = {.vertex_array = init_vao()};

	const plane_type_t origin[3] = {2, 2, 5}, size[3] = {3, 2, 8};
	// plane_type_t* const cuboid_mesh = create_sector_mesh(origin, size);
	plane_type_t* const cuboid_mesh = malloc(bytes_per_mesh);
	create_sector_mesh(origin, size, cuboid_mesh);

	sgl.num_vertex_buffers = 1;
	sgl.vertex_buffers = init_vbos(sgl.num_vertex_buffers, cuboid_mesh, bytes_per_mesh);
	free(cuboid_mesh);
	bind_interleaved_planes_to_vao();

	sgl.shader_program = init_shader_program(demo_4_vertex_shader, demo_4_fragment_shader);
	sgl.num_textures = 1;
	sgl.textures = init_textures(sgl.num_textures, "../../../assets/walls/mesa.bmp", tex_repeating);
	select_texture_for_use(sgl.textures[0], sgl.shader_program);
	enable_all_culling();

	return sgl;
}

void demo_11_drawer(const StateGL* const sgl) {
	move(sgl -> shader_program);
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f); // Dark blue
	enum {num_meshes = 1};
	draw_triangles(num_meshes * triangles_per_mesh);
}

#ifdef DEMO_11
int main(void) {
	make_application(demo_11_drawer, demo_11_init, deinit_demo_vars);
}
#endif
