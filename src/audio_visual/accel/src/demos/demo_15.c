#include "../utils.c"
#include "../camera.c"
#include "../shaders.c"

// http://www.humus.name/index.php?page=Textures

const GLfloat skybox_vertices[] = {
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

// This reads a skybox file into a OpenGL cubemap texture
GLuint init_skybox_texture(const char* const path) {
	SDL_Surface* const skybox_surface = init_surface(path);
	SDL_UnlockSurface(skybox_surface);
	const GLint cube_size = skybox_surface -> w >> 2;

	GLuint skybox;
	glGenTextures(1, &skybox);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, OPENGL_TEX_MAG_FILTER);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, OPENGL_SKYBOX_TEX_MIN_FILTER);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, tex_nonrepeating);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, tex_nonrepeating);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, tex_nonrepeating);

	SDL_Surface* const face_surface = SDL_CreateRGBSurfaceWithFormat(0, cube_size, cube_size,
		cube_size * sizeof(Uint32), SDL_PIXEL_FORMAT);

	void* const face_pixels = face_surface -> pixels;
	SDL_Rect dest_rect = {0, 0, cube_size, cube_size};

	typedef struct {int x, y;} ivec2;
	ivec2 src_origins[6];

	// right, left, top, bottom, back, front
	const GLint twice_cube_size = cube_size << 1;
	src_origins[0] = (ivec2) {twice_cube_size, cube_size};
	src_origins[1] = (ivec2) {0, cube_size};
	src_origins[2] = (ivec2) {cube_size, 0};
	src_origins[3] = (ivec2) {cube_size, twice_cube_size};
	src_origins[4] = (ivec2) {cube_size, cube_size};
	src_origins[5] = (ivec2) {twice_cube_size + cube_size, cube_size};

	for (byte i = 0; i < 6; i++) {
		const ivec2 src_origin = src_origins[i];
		SDL_Rect src_rect = {src_origin.x, src_origin.y, cube_size, cube_size};

		SDL_LowerBlit(skybox_surface, &src_rect, face_surface, &dest_rect);
		SDL_LockSurface(face_surface); // Locking for read access to face_pixels

		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
			0, OPENGL_INTERNAL_PIXEL_FORMAT,
			cube_size, cube_size, 0, OPENGL_INPUT_PIXEL_FORMAT,
			OPENGL_COLOR_CHANNEL_TYPE, face_pixels);

		SDL_UnlockSurface(face_surface); // Unlocking for next call to SDL_LowerBlit
	}

	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	deinit_surface(face_surface);
	deinit_surface(skybox_surface);

	return skybox;
}

StateGL demo_15_init(void) {
	StateGL sgl = {.vertex_array = init_vao()};

	sgl.num_vertex_buffers = 1;
	sgl.vertex_buffers = init_vbos(sgl.num_vertex_buffers, skybox_vertices, sizeof(skybox_vertices));
	bind_vbos_to_vao(sgl.vertex_buffers, sgl.num_vertex_buffers, 3);

	sgl.shader_program = init_shader_program(skybox_vertex_shader, skybox_fragment_shader);
	glUseProgram(sgl.shader_program);

	//////////
	sgl.num_textures = 0;
	const GLuint skybox_texture = init_skybox_texture("assets/night.bmp");
	sgl.any_data = (void*) (uint64_t) skybox_texture;

	const GLuint shader_texture_sampler = glGetUniformLocation(sgl.shader_program, "texture_sampler");
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_texture);
	glUniform1i(shader_texture_sampler, 0);
	//////////

	glDepthFunc(GL_LEQUAL);

	return sgl;
}

void demo_15_drawer(const StateGL* const sgl) {
	static Camera camera;
	static GLint view_projection_id;
	static byte first_call = 1;

	if (first_call) {
		init_camera(&camera, (vec3) {0.0f, 0.0f, 0.0f});
		view_projection_id = glGetUniformLocation(sgl -> shader_program, "view_projection");
		first_call = 0;
	}

	update_camera(&camera);

	/* Clears x, y, and w; z (depth) is not cleared b/c it's always set to 1
	in the vertex shader. If this matrix is modified here last, it's okay
	because it will be newly generated at the next update_camera call. */
	camera.view_projection[3][0] = 0.0f;
	camera.view_projection[3][1] = 0.0f;
	camera.view_projection[3][3] = 0.0f;

	glUniformMatrix4fv(view_projection_id, 1, GL_FALSE, &camera.view_projection[0][0]);

	glDepthMask(GL_FALSE);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glDepthMask(GL_TRUE);
}

#ifdef DEMO_15
int main(void) {
	make_application(demo_15_drawer, demo_15_init, deinit_demo_vars);
}
#endif
