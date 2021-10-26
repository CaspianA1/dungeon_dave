#include "demo_5.c"

/*
Functions to make:
- void create_cuboid(const vec3 origin, const vec3 size);
- GLfloat* create_hori_plane(const vec3 top_left_corner, const int width, const int height);
- GLfloat* join_plane_vertices(const GLfloat* const plane_1, const GLfloat* const plane_2);

Other stuff:
- send integral points to the gpu + plane functions top left corner -> just ints
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
	const int width, const int height)

PLANE_CREATOR_FUNCTION(vert) {
	const float top_left_x = top_left_corner[0], top_left_y = top_left_corner[1], z = top_left_corner[2];
	const float top_right_x = top_left_x + width, bottom_left_y = top_left_y - height;

	const GLfloat vertices[plane_vertex_floats] = {
		top_left_x, top_left_y, z,
		top_right_x, top_left_y, z,
		top_left_x, bottom_left_y, z,

		top_left_x, bottom_left_y, z,
		top_right_x, bottom_left_y, z,
		top_right_x, top_left_y, z
	};

	GLfloat* const vertex_data = malloc(plane_vertex_bytes);
	memcpy(vertex_data, vertices, plane_vertex_bytes);
	return vertex_data;
}

StateGL demo_6_init(void) {
	StateGL sgl;

	sgl.vertex_array = init_vao();
	sgl.index_buffer = init_ibo(demo_3_index_data, sizeof(demo_3_index_data));

	vec3 v_top_left_corner = {3.0f, 3.0f, -1.0f};
	const int v_width = 2, v_height = 3;

	GLfloat
		*const flat_square_vertices = create_vert_plane(v_top_left_corner, v_width, v_height),
		*const uv_data = create_uv_for_plane(v_width, v_height);

	sgl.num_vertex_buffers = 2;
	sgl.vertex_buffers = init_vbos(sgl.num_vertex_buffers,
		flat_square_vertices, plane_vertex_bytes,
		uv_data, plane_uv_bytes);
	
	free(flat_square_vertices);
	free(uv_data);
	
	sgl.shader_program = init_shader_program(demo_4_vertex_shader, demo_4_fragment_shader);

	sgl.num_textures = 1;
	sgl.textures = init_textures(sgl.num_textures, "assets/walls/cobblestone_3.bmp");
	select_texture_for_use(sgl.textures[0], sgl.shader_program);
	
	return sgl;
}

#ifdef DEMO_6
int main(void) {
    make_application(demo_5_drawer, demo_6_init, deinit_demo_vars);
}
#endif
