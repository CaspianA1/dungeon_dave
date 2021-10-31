#include "demo_5.c"

// demo 10, but just with triangles in correct winding order
// fix uv flip

typedef GLubyte plane_type_t;
#define PLANE_TYPE_ENUM GL_UNSIGNED_BYTE

// vert per plane 6 before
enum {vars_per_vertex = 5, vertices_per_plane = 24, planes_per_mesh = 4};
enum {vars_per_plane = vars_per_vertex * vertices_per_plane};
enum {bytes_per_plane = vars_per_plane * sizeof(plane_type_t)};
enum {bytes_per_sector_mesh = bytes_per_plane * planes_per_mesh};

void check_for_mesh_out_of_bounds(const plane_type_t origin[3], const plane_type_t size[3]) {
	for (byte i = 0; i < 3; i++) {
		const GLfloat start = origin[i], length = size[i];
		const GLfloat end = start + ((i == 1) ? -length : length);

		if (start < 0.0f || start > 255.0f || end < 0.0f || end > 255.0f) {
			fprintf(stderr, "Mesh out of bounds on %c axis\n", 'x' + i);
			fail("create mesh: mesh out of bounds", MeshOutOfBounds);
		}
	}
}

plane_type_t* create_sector_mesh(const plane_type_t origin[3], const plane_type_t size[3]) {
	check_for_mesh_out_of_bounds(origin, size);

	plane_type_t* const sector_mesh = malloc(bytes_per_sector_mesh);

	const plane_type_t
		size_x = size[0], size_y = size[1], size_z = size[2],
		near_x = origin[0], top_y = origin[1], near_z = origin[2];

	const plane_type_t far_x = near_x + size_x, bottom_y = top_y - size_y, far_z = near_z + size_z;

	const plane_type_t vertices[vars_per_plane] = {
		// Top triangles aligned along Z axis (each pair in opposite winding order from each other)
		near_x, bottom_y, near_z, 0.0f, size_y,
		near_x, top_y, far_z, size_z, 0.0f,
		near_x, top_y, near_z, 0.0f, 0.0f,

		far_x, top_y, near_z, 0.0f, 0.0f,
		far_x, top_y, far_z, size_z, 0.0f,
		far_x, bottom_y, near_z, 0.0f, size_y,

		// Bottom triangles aligned along Z axis
		near_x, bottom_y, near_z, 0.0f, size_y,
		near_x, bottom_y, far_z, size_z, size_y,
		near_x, top_y, far_z, size_z, 0.0f,

		far_x, top_y, far_z, size_z, 0.0f,
		far_x, bottom_y, far_z, size_z, size_y,
		far_x, bottom_y, near_z, 0.0f, size_y,

		// Top triangles aligned along X axis
		near_x, top_y, near_z, 0.0f, 0.0f,
		far_x, top_y, near_z, size_x, 0.0f,
		near_x, bottom_y, near_z, 0.0f, size_y,

		near_x, bottom_y, far_z, 0.0f, size_y,
		far_x, top_y, far_z, size_x, 0.0f,
		near_x, top_y, far_z, 0.0f, 0.0f,

		// Bottom triangles aligned along X axis
		far_x, top_y, near_z, size_x, 0.0f,
		far_x, bottom_y, near_z, size_x, size_y,
		near_x, bottom_y, near_z, 0.0f, size_y,

		near_x, bottom_y, far_z, 0.0f, size_y,
		far_x, bottom_y, far_z, size_x, size_y,
		far_x, top_y, far_z, size_x, 0.0f
	};

	memcpy(sector_mesh, vertices, sizeof(vertices));

	return sector_mesh;
}

void bind_interleaved_planes_to_vao(void) {
	enum {bytes_per_vertex = vars_per_vertex * sizeof(plane_type_t)};

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, PLANE_TYPE_ENUM, GL_FALSE, bytes_per_vertex, NULL);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, PLANE_TYPE_ENUM, GL_FALSE, bytes_per_vertex, (void*) (3 * sizeof(plane_type_t)));
}

StateGL demo_11_init(void) {
	StateGL sgl = {.vertex_array = init_vao()};

	const plane_type_t origin[3] = {5, 5, 10}, size[3] = {1, 2, 8};
	plane_type_t* const cuboid_mesh = create_sector_mesh(origin, size);

	sgl.num_vertex_buffers = 1;
	sgl.vertex_buffers = init_vbos(sgl.num_vertex_buffers, cuboid_mesh, bytes_per_sector_mesh);
	bind_interleaved_planes_to_vao();
	free(cuboid_mesh);

	sgl.shader_program = init_shader_program(demo_4_vertex_shader, demo_4_fragment_shader);
	sgl.num_textures = 1;
	sgl.textures = init_textures(sgl.num_textures, "../../../assets/walls/mesa.bmp");
	select_texture_for_use(sgl.textures[0], sgl.shader_program);
	enable_all_culling();

	return sgl;
}

void demo_11_drawer(const StateGL* const sgl) {
	move(sgl -> shader_program);
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f); // Dark blue
	enum {num_meshes = 1};
	draw_triangles(num_meshes * planes_per_mesh * 2); // 2 = 2 triangles per plane
}

#ifdef DEMO_11
int main(void) {
	make_application(demo_11_drawer, demo_11_init, deinit_demo_vars);
}
#endif
