#include "demo_5.c"

/*
Functions to make:
- void create_cuboid(const vec3 origin, const vec3 size);
- GLfloat* create_hori_plane(const vec3 top_left_corner, const int width, const int height);
- GLfloat* join_plane_vertices(const GLfloat* const plane_1, const GLfloat* const plane_2);

Other stuff:
- send integral points to the gpu + plane functions top left corner -> just ints
*/

/*
0__1
|  /
| /
2/
*/

enum {plane_vertex_floats = 18, plane_uv_floats = 12};

const size_t
	plane_vertex_bytes = plane_vertex_floats * sizeof(GLfloat),
	plane_uv_bytes = plane_uv_floats * sizeof(GLfloat);

GLfloat* create_uv_for_plane(const int width, const int height) {
	const GLfloat uv[plane_uv_floats] = {
		0.0f, 0.0f,
		width, 0.0f,
		0.0f, height,

		0.0f, height,
		width, height,
		width, 0.0f
	};

	GLfloat* const uv_data = malloc(plane_uv_bytes);
	memcpy(uv_data, uv, plane_uv_bytes);
	return uv_data;
}

#define PLANE_CREATOR_FUNCTION(type) GLfloat* create_##type##_plane(const vec3 top_left_corner,\
	const int size_hori, const int size_vert)

PLANE_CREATOR_FUNCTION(vert) {
	const float left_x = top_left_corner[0], top_y = top_left_corner[1], z = top_left_corner[2];
	const float right_x = left_x + size_hori, bottom_y = top_y - size_vert;

	const GLfloat vertices[plane_vertex_floats] = {
		left_x, top_y, z,
		right_x, top_y, z,
		left_x, bottom_y, z,

		left_x, bottom_y, z,
		right_x, bottom_y, z,
		right_x, top_y, z
	};

	GLfloat* const vertex_data = malloc(plane_vertex_bytes);
	memcpy(vertex_data, vertices, plane_vertex_bytes);
	return vertex_data;
}

PLANE_CREATOR_FUNCTION(hori) {
	const float left_x = top_left_corner[0], height = top_left_corner[1], depth_origin = top_left_corner[2];
	const float right_x = left_x + size_hori, largest_depth = depth_origin + size_vert;

	const GLfloat vertices[plane_vertex_floats] = {
		left_x, height, depth_origin,
		right_x, height, depth_origin,
		left_x, height, largest_depth,

		left_x, height, largest_depth,
		right_x, height, largest_depth,
		right_x, height, depth_origin
	};

	GLfloat* const vertex_data = malloc(plane_vertex_bytes);
	memcpy(vertex_data, vertices, plane_vertex_bytes);
	return vertex_data;
}

StateGL demo_6_init(void) {
	StateGL sgl;

	sgl.vertex_array = init_vao();
	sgl.index_buffer = init_ibo(demo_3_index_data, sizeof(demo_3_index_data));

	const vec3 top_left_corner = {-5.0f, -2.0f, 1.0f};
	const int size_hori = 2, size_vert = 3;

	GLfloat
		*const plane_vertices = create_hori_plane(top_left_corner, size_hori, size_vert),
		*const uv_data = create_uv_for_plane(size_hori, size_vert);

	sgl.num_vertex_buffers = 2;
	sgl.vertex_buffers = init_vbos(sgl.num_vertex_buffers,
		plane_vertices, plane_vertex_bytes,
		uv_data, plane_uv_bytes);

	free(plane_vertices);
	free(uv_data);
	
	sgl.shader_program = init_shader_program(demo_4_vertex_shader, demo_4_fragment_shader);

	sgl.num_textures = 1;
	sgl.textures = init_textures(sgl.num_textures, "assets/walls/saqqara.bmp");
	select_texture_for_use(sgl.textures[0], sgl.shader_program);
	
	return sgl;
}

#ifdef DEMO_6
int main(void) {
    make_application(demo_5_drawer, demo_6_init, deinit_demo_vars);
}
#endif
