#include "demo_5.c"

/*
- sectors must still be drawn separately
- may be faster for GPU with more data and nothing fancy
- sector making fn
- fix culling by a function called rearrange_mesh_for_culling
*/

typedef GLfloat plane_type_t;
#define PLANE_TYPE_ENUM GL_FLOAT

//////////

#define PLANE_CREATOR_NAME(type) create_##type##_plane

#define PLANE_CREATOR_SIGNATURE const plane_type_t top_left_corner[3],\
	const plane_type_t size_hori, const plane_type_t size_vert, plane_type_t* const dest

#define PLANE_CREATOR_FUNCTION(type) void PLANE_CREATOR_NAME(type)(PLANE_CREATOR_SIGNATURE)

//////////

enum {vars_per_vertex = 5, vertices_per_plane = 6, planes_per_mesh = 5};
enum {vars_per_plane = vars_per_vertex * vertices_per_plane};
enum {bytes_per_plane = vars_per_plane * sizeof(plane_type_t)};
enum {bytes_per_sector_mesh = bytes_per_plane * planes_per_mesh};
// Sectors are meshes here

//////////

PLANE_CREATOR_FUNCTION(vert_1) { // Aligned on X axis
	const plane_type_t left_x = top_left_corner[0], top_y = top_left_corner[1], z = top_left_corner[2];
	const plane_type_t right_x = left_x + size_hori, bottom_y = top_y - size_vert;

	const plane_type_t vertices[vars_per_plane] = {
		left_x, top_y, z, 0.0f, 0.0f,
		right_x, top_y, z, size_hori, 0.0f,
		left_x, bottom_y, z, 0.0f, size_vert,

		left_x, bottom_y, z, 0.0f, size_vert,
		right_x, bottom_y, z, size_hori, size_vert,
		right_x, top_y, z, size_hori, 0.0f
	};

	memcpy(dest, vertices, bytes_per_plane);
}

PLANE_CREATOR_FUNCTION(vert_2) { // Aligned on Z axis
	const plane_type_t x = top_left_corner[0], top_y = top_left_corner[1], far_z = top_left_corner[2];
	const plane_type_t bottom_y = top_y - size_vert, near_z = far_z + size_hori;

	const plane_type_t vertices[vars_per_plane] = {
		x, top_y, far_z, 0.0f, 0.0f,
		x, top_y, near_z, size_hori, 0.0f,
		x, bottom_y, far_z, 0.0f, size_vert,

		x, bottom_y, far_z, 0.0f, size_vert,
		x, bottom_y, near_z, size_hori, size_vert,
		x, top_y, near_z, size_hori, 0.0f
	};

	memcpy(dest, vertices, bytes_per_plane);
}

PLANE_CREATOR_FUNCTION(hori) {
	const plane_type_t left_x = top_left_corner[0], height = top_left_corner[1], depth_origin = top_left_corner[2];
	const plane_type_t right_x = left_x + size_hori, largest_depth = depth_origin + size_vert;

	const plane_type_t vertices[vars_per_plane] = {
		left_x, height, depth_origin, 0.0f, 0.0f,
		right_x, height, depth_origin, size_hori, 0.0f,
		left_x, height, largest_depth, 0.0f, size_vert,

		left_x, height, largest_depth, 0.0f, size_vert,
		right_x, height, largest_depth, size_hori, size_vert,
		right_x, height, depth_origin, size_hori, 0.0f
	};

	memcpy(dest, vertices, bytes_per_plane);
}

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

// origin = top left corner
plane_type_t* generate_sector_mesh(plane_type_t origin[3], const plane_type_t size[3]) {
	check_for_mesh_out_of_bounds(origin, size);

	plane_type_t* const sector_mesh = malloc(bytes_per_sector_mesh);

	PLANE_CREATOR_NAME(vert_2)(origin, size[2], size[1], sector_mesh);
	origin[0] += size[0];
	PLANE_CREATOR_NAME(vert_2)(origin, size[2], size[1], sector_mesh + vars_per_plane * 1);
	origin[0] -= size[0];

	// only the top horizontal plane; bottom one will always be invisible on map
	PLANE_CREATOR_NAME(hori)(origin, size[0], size[2], sector_mesh + vars_per_plane * 2);

	PLANE_CREATOR_NAME(vert_1)(origin, size[0], size[1], sector_mesh + vars_per_plane * 3);
	origin[2] += size[2];
	PLANE_CREATOR_NAME(vert_1)(origin, size[0], size[1], sector_mesh + vars_per_plane * 4);
	origin[2] -= size[2];

	return sector_mesh;
}

void bind_interleaved_planes_to_vao(void) {
	enum {bytes_per_vertex = vars_per_vertex * sizeof(plane_type_t)};

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, PLANE_TYPE_ENUM, GL_FALSE, bytes_per_vertex, NULL);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, PLANE_TYPE_ENUM, GL_FALSE, bytes_per_vertex, (void*) (3 * sizeof(plane_type_t)));
}

StateGL demo_10_init(void) {
	StateGL sgl = {.vertex_array = init_vao()};

	plane_type_t origin[3] = {5, 1, 5}, size[3] = {250, 1, 250};
	plane_type_t* const sector_mesh = generate_sector_mesh(origin, size);

	sgl.num_vertex_buffers = 1;
	sgl.vertex_buffers = init_vbos(sgl.num_vertex_buffers, sector_mesh, bytes_per_sector_mesh);
	bind_interleaved_planes_to_vao();

	free(sector_mesh);
	
	sgl.shader_program = init_shader_program(demo_4_vertex_shader, demo_4_fragment_shader);
	glUseProgram(sgl.shader_program);

	sgl.num_textures = 1;
	sgl.textures = init_plain_textures(sgl.num_textures, "../../../../assets/walls/mesa.bmp", TexRepeating);
	use_texture(sgl.textures[0], sgl.shader_program, TexPlain);

	enable_all_culling();

	return sgl;
}

void demo_10_drawer(const StateGL* const sgl) {
	move(sgl -> shader_program);
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f); // Dark blue
	enum {num_meshes = 1};

	const GLsizei num_triangles = num_meshes * planes_per_mesh * 2; // 2 = 2 triangles per plane
	glDrawArrays(GL_TRIANGLES, 0, num_triangles * 3);
}

#ifdef DEMO_10
int main(void) {
	make_application(demo_10_drawer, demo_10_init, deinit_demo_vars);
}
#endif
