#include "utils.c"
#include "demo_3.c"

// Next step: texture cube with whole texture on each side

StateGL demo_4_init(void) {
	StateGL sgl;

	sgl.vertex_array = init_vao();

	enum {floats_per_uv = 2, uvs_per_triangle = 3};
	enum {num_uv_floats = triangles_per_cube * uvs_per_triangle * floats_per_uv};

	static const GLfloat uv_data[num_uv_floats] = {
		0.000059f, 1.0f-0.000004f,
		0.000103f, 1.0f-0.336048f,
		0.335973f, 1.0f-0.335903f,
		1.000023f, 1.0f-0.000013f,
		0.667979f, 1.0f-0.335851f,
		0.999958f, 1.0f-0.336064f,
		0.667979f, 1.0f-0.335851f,
		0.336024f, 1.0f-0.671877f,
		0.667969f, 1.0f-0.671889f,
		1.000023f, 1.0f-0.000013f,
		0.668104f, 1.0f-0.000013f,
		0.667979f, 1.0f-0.335851f,
		0.000059f, 1.0f-0.000004f,
		0.335973f, 1.0f-0.335903f,
		0.336098f, 1.0f-0.000071f,
		0.667979f, 1.0f-0.335851f,
		0.335973f, 1.0f-0.335903f,
		0.336024f, 1.0f-0.671877f,
		1.000004f, 1.0f-0.671847f,
		0.999958f, 1.0f-0.336064f,
		0.667979f, 1.0f-0.335851f,
		0.668104f, 1.0f-0.000013f,
		0.335973f, 1.0f-0.335903f,
		0.667979f, 1.0f-0.335851f,
		0.335973f, 1.0f-0.335903f,
		0.668104f, 1.0f-0.000013f,
		0.336098f, 1.0f-0.000071f,
		0.000103f, 1.0f-0.336048f,
		0.000004f, 1.0f-0.671870f,
		0.336024f, 1.0f-0.671877f,
		0.000103f, 1.0f-0.336048f,
		0.336024f, 1.0f-0.671877f,
		0.335973f, 1.0f-0.335903f,
		0.667969f, 1.0f-0.671889f,
		1.000004f, 1.0f-0.671847f,
		0.667979f, 1.0f-0.335851f
	};

	sgl.num_vertex_buffers = 2;
	sgl.vertex_buffers = init_vbos(sgl.num_vertex_buffers,
		demo_3_vertex_data, sizeof(demo_3_vertex_data),
		uv_data, sizeof(uv_data));

	const char* const vertex_shader =
		"#version 330 core\n"
		"layout(location = 0) in vec3 vertexPosition_modelspace;\n"
		"layout(location = 1) in vec2 vertexUV;\n"
		"out vec2 UV;\n"
		"uniform mat4 MVP;\n"
		"void main() {\n"
			"gl_Position = MVP * vec4(vertexPosition_modelspace, 1);\n"
			"UV = vertexUV;\n"
		"}\n",

	*const fragment_shader =
		"#version 330 core\n"
		"in vec2 UV;\n"
		"out vec3 color;\n" // For textures with an alpha channel, enable 4 channels
		"uniform sampler2D myTextureSampler;\n"
		"void main() {\n"
			"color = texture(myTextureSampler, UV).rgb;\n"
		"}\n";

	sgl.shader_program = init_shader_program(vertex_shader, fragment_shader);

	// "src/audio_visual/accel/tutorial_test/uv.bmp"
	// "assets/walls/saqqara.bmp"

	sgl.num_textures = 1;
	sgl.textures = init_textures(sgl.num_textures, "assets/walls/dial.bmp");

	// For textures with an alpha channel, enable this
	/* glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); */

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	return sgl;
}

void demo_4_drawer(const StateGL sgl) {
	static vec3 camera_pos = {-2.0f, 0.5f, 0.0f};
	const double step = 0.05;
	if (keys[SDL_SCANCODE_W]) camera_pos[0] += step;
	if (keys[SDL_SCANCODE_S]) camera_pos[0] -= step;
	if (keys[SDL_SCANCODE_1]) camera_pos[1] += step;
	if (keys[SDL_SCANCODE_2]) camera_pos[1] -= step;
	if (keys[SDL_SCANCODE_A]) camera_pos[2] += step;
	if (keys[SDL_SCANCODE_D]) camera_pos[2] -= step;

	demo_2_matrix_setup(sgl.shader_program, camera_pos);

	const GLuint shader_texture_sampler = glGetUniformLocation(sgl.shader_program, "myTextureSampler");
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, sgl.textures[0]); // set the current bound texture
	glUniform1i(shader_texture_sampler, 0); // make the sampler read from texture unit 0

	glClearColor(0.0f, 0.0f, 0.4f, 0.0f); // Dark blue
	bind_vbos_to_vao(sgl.vertex_buffers, sgl.num_vertex_buffers, 3, 2);
	draw_triangles(12);
	unbind_vbos_from_vao(sgl.num_vertex_buffers);
}

#ifdef DEMO_4
int main(void) {
	make_application(demo_4_drawer, demo_4_init, deinit_demo_vars);
}
#endif
