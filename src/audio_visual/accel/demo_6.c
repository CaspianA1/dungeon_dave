#include "demo_5.c"

/*
Functions:
- create_rect_plane_vertices
- create_cuboid_vertices
*/

void create_rect_plane_vertices(const int width, const int height,
	GLfloat** const vertex_data, GLfloat** const uv_data) {

	const size_t vertex_bytes = 18 * sizeof(GLfloat), uv_bytes = 12 * sizeof(GLfloat);
	*vertex_data = malloc(vertex_bytes), *uv_data = malloc(uv_bytes);
	const float s = 20.0f; // s = size

	const GLfloat vertices[] = {
		-s, s, 0.0f,
		s, s, 0.0f,
		-s, -s, 0.0f,

		-s, -s, 0.0f,
		s, -s, 0.0f,
		s, s, 0.0f
	},

	uv[] = {
		0.0f, 0.0f,
		width, 0.0f,
		0.0f, height,

		0.0f, height,
		width, height,
		width, 0.0f
	};

	memcpy(*vertex_data, vertices, vertex_bytes);
	memcpy(*uv_data, uv, uv_bytes);
}

void create_cuboid_vertices(vec3 origin, vec3 size);

StateGL demo_6_init(void) {
	StateGL sgl;

	sgl.vertex_array = init_vao();
	sgl.index_buffer = init_ibo(demo_3_index_data, sizeof(demo_3_index_data));

	GLfloat *flat_square_vertices, *uv_data;
	create_rect_plane_vertices(3, 5, &flat_square_vertices, &uv_data);

	sgl.num_vertex_buffers = 2;
	sgl.vertex_buffers = init_vbos(sgl.num_vertex_buffers,
		flat_square_vertices, 18 * sizeof(GLfloat),
		uv_data, 12 * sizeof(GLfloat));
	
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
