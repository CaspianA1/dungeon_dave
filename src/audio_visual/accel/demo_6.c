#include "demo_5.c"

/*
Functions:
- create_rect_plane_vertices
- create_cuboid_vertices
- send integral points to the gpu
*/

const size_t
	plane_vertex_bytes = 18 * sizeof(GLfloat),
	plane_uv_bytes = 12 * sizeof(GLfloat);

void create_uv_for_plane(const int width, const int height, GLfloat** const uv_data) {
	*uv_data = malloc(plane_uv_bytes);

	const GLfloat uv[] = {
		0.0f, 0.0f,
		width, 0.0f,
		0.0f, height,

		0.0f, height,
		width, height,
		width, 0.0f
	};

	memcpy(*uv_data, uv, plane_uv_bytes);
}

// top_left_corner should be composed of just integers
void create_vert_rect_plane(const vec3 top_left_corner, const int width, const int height,
	GLfloat** const vertex_data) {

	*vertex_data = malloc(plane_vertex_bytes);

	const float top_left_x = top_left_corner[0], top_left_y = top_left_corner[1], z = top_left_corner[2];
	const float top_right_x = top_left_x + width, bottom_left_y = top_left_y - height;

	const GLfloat vertices[] = {
		top_left_x, top_left_y, z,
		top_right_x, top_left_y, z,
		top_left_x, bottom_left_y, z,

		top_left_x, bottom_left_y, z,
		top_right_x, bottom_left_y, z,
		top_right_x, top_left_y, z
	};

	memcpy(*vertex_data, vertices, plane_vertex_bytes);
}

void create_hori_rect_plane(const vec3 top_left_corner, const int width, const int height,
	GLfloat** const vertex_data) {

	(void) top_left_corner;
	(void) width;
	(void) height;
	(void) vertex_data;
}

void create_cuboid(vec3 origin, vec3 size);

StateGL demo_6_init(void) {
	StateGL sgl;

	sgl.vertex_array = init_vao();
	sgl.index_buffer = init_ibo(demo_3_index_data, sizeof(demo_3_index_data));

	GLfloat *flat_square_vertices, *uv_data;
	vec3 top_left_corner = {3.0f, 1.0f, -1.0f};
	const int width = 2, height = 3;
	create_vert_rect_plane(top_left_corner, width, height, &flat_square_vertices);
	create_uv_for_plane(width, height, &uv_data);

	sgl.num_vertex_buffers = 2;
	sgl.vertex_buffers = init_vbos(sgl.num_vertex_buffers,
		flat_square_vertices, plane_vertex_bytes,
		uv_data, plane_uv_bytes);
	
	free(flat_square_vertices);
	free(uv_data);
	
	sgl.shader_program = init_shader_program(demo_4_vertex_shader, demo_4_fragment_shader);

	sgl.num_textures = 1;
	sgl.textures = init_textures(sgl.num_textures, "assets/walls/saqqara.bmp");

	const GLuint shader_texture_sampler = glGetUniformLocation(sgl.shader_program, "texture_sampler");
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, sgl.textures[0]); // Set the current bound texture
	glUniform1i(shader_texture_sampler, 0); // Make the sampler read from texture unit 0
	
	return sgl;
}

#ifdef DEMO_6
int main(void) {
    make_application(demo_5_drawer, demo_6_init, deinit_demo_vars);
}
#endif
