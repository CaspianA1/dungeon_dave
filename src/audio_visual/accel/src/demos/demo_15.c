#include "../utils.c"
#include "../sector.c"
#include "../batch.c"
#include "../camera.c"

/* right = GL_TEXTURE_CUBE_MAP_POSITIVE_X
left = GL_TEXTURE_CUBE_MAP_NEGATIVE_X

top = GL_TEXTURE_CUBE_MAP_POSITIVE_Y
bottom = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y

back = GL_TEXTURE_CUBE_MAP_POSITIVE_Z
front = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z */

const char* const demo_15_vertex_shader =
	"#version 330 core\n"

	"layout(location = 0) in vec3 vertex_pos_model_space;\n"

	"out vec3 UV_3D;\n"
	"uniform mat4 VP;\n"

	"void main() {\n"
		"gl_Position = VP * vec4(vertex_pos_model_space, 1.0);\n"
		"gl_Position = gl_Position.xyww;\n"
		"UV_3D = vertex_pos_model_space;\n"
	"}\n"
	,

*const demo_15_fragment_shader =
	"#version 330 core\n"
	"in vec3 UV_3D;\n"
	"uniform samplerCube cubemap_sampler;\n"
	"out vec4 color;\n"

	"void main() {\n"
		"color = texture(cubemap_sampler, UV_3D);\n"
	"}\n";

// Skybox is a cubemap
GLuint init_skybox_texture(const char* const path) {
	const char* const paths[6] = {
		"assets/right.bmp",
		"assets/left.bmp",
		"assets/top.bmp",
		"assets/bottom.bmp",
		"assets/front.bmp",
		"assets/back.bmp"
	};

	(void) path;
	GLuint skybox;
	glGenTextures(1, &skybox);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, OPENGL_TEX_MAG_FILTER);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, OPENGL_SKYBOX_TEX_MIN_FILTER);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, tex_nonrepeating);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, tex_nonrepeating);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, tex_nonrepeating);

	for (byte i = 0; i < 6; i++) {
		SDL_Surface* const surface = init_surface(paths[i]);

		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
			0, OPENGL_INTERNAL_PIXEL_FORMAT,
			surface -> w, surface -> h, 0, OPENGL_INPUT_PIXEL_FORMAT,
			OPENGL_COLOR_CHANNEL_TYPE, surface -> pixels);

		deinit_surface(surface);
	}

	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	return skybox;
}

StateGL demo_15_init(void) {
	StateGL sgl = {.vertex_array = init_vao()};

	GLfloat vertices[] = {
		-1.0f, 1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, 1.0f, -1.0f,
		-1.0f, 1.0f, -1.0f,

		-1.0f, -1.0f, 1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f, 1.0f, -1.0f,
		-1.0f, 1.0f, -1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f, -1.0f, 1.0f,

		1.0f, -1.0f, -1.0f,
	 	1.0f, -1.0f, 1.0f,
	 	1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
	 	1.0f, 1.0f, -1.0f,
	 	1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, -1.0f, 1.0f,
		-1.0f, -1.0f, 1.0f,

		-1.0f, 1.0f, -1.0f,
		1.0f, 1.0f, -1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f, 1.0f,
	 	1.0f, -1.0f, -1.0f,
	 	1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f, 1.0f,
	 	1.0f, -1.0f, 1.0f
	};

	sgl.num_vertex_buffers = 1;
	sgl.vertex_buffers = init_vbos(sgl.num_vertex_buffers, vertices, sizeof(vertices));
	bind_vbos_to_vao(sgl.vertex_buffers, sgl.num_vertex_buffers, 3);

	sgl.shader_program = init_shader_program(demo_15_vertex_shader, demo_15_fragment_shader);

	//////////
	sgl.num_textures = 0; // "../../../assets/walls/saqqara.bmp"
	const GLuint skybox_texture = init_skybox_texture("assets/skybox_1.bmp");
	sgl.any_data = (void*) (uint64_t) skybox_texture;

	const GLuint shader_texture_sampler = glGetUniformLocation(sgl.shader_program, "texture_sampler");
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_texture);
	glUniform1i(shader_texture_sampler, 0);
	//////////

	glDepthMask(GL_FALSE);
	glDepthFunc(GL_LEQUAL);

	return sgl;
}

void demo_15_drawer(const StateGL* const sgl) {
	static byte first_call = 1;
	static Camera camera;
	static GLint view_projection_id;

	if (first_call) {
		init_camera(&camera, (vec3) {0.0f, 0.0f, 0.0f});
		view_projection_id = glGetUniformLocation(sgl -> shader_program, "VP");
		first_call = 0;
	}

	update_camera(&camera);

	/* Clears x, y, and w; z, depth, is not cleared b/c it's always set to 1
	in the vertex shader. If this matrix is modified here last, it's okay
	because it will be newly generated at the next update_camera call. */
	camera.view_projection[3][0] = 0.0f;
	camera.view_projection[3][1] = 0.0f;
	camera.view_projection[3][3] = 0.0f;

	glUniformMatrix4fv(view_projection_id, 1, GL_FALSE, &camera.view_projection[0][0]);
	glDrawArrays(GL_TRIANGLES, 0, 36);
}

#ifdef DEMO_15
int main(void) {
	make_application(demo_15_drawer, demo_15_init, deinit_demo_vars);
}
#endif
