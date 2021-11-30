#ifndef SKYBOX_C
#define SKYBOX_C

#include "headers/skybox.h"

const GLbyte skybox_vertices[] = {
	-1, 1, -1,
	-1, -1, -1,
	1, -1, -1,
	1, -1, -1,
	1, 1, -1,
	-1, 1, -1,

	-1, -1, 1,
	-1, -1, -1,
	-1, 1, -1,
	-1, 1, -1,
	-1, 1, 1,
	-1, -1, 1,

	1, -1, -1,
	1, -1, 1,
	1, 1, 1,
	1, 1, 1,
	1, 1, -1,
	1, -1, -1,

	-1, -1, 1,
	-1, 1, 1,
	1, 1, 1,
	1, 1, 1,
	1, -1, 1,
	-1, -1, 1,

	-1, 1, -1,
	1, 1, -1,
	1, 1, 1,
	1, 1, 1,
	-1, 1, 1,
	-1, 1, -1,

	-1, -1, -1,
	-1, -1, 1,
	1, -1, -1,
	1, -1, -1,
	-1, -1, 1,
	1, -1, 1
};

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

	const GLint twice_cube_size = cube_size << 1;

	typedef struct {const GLint x, y;} ivec2;

	// right, left, top, bottom, back, front
	const ivec2 src_origins[6] = {
		{twice_cube_size, cube_size},
		{0, cube_size},
		{cube_size, 0},
		{cube_size, twice_cube_size},
		{cube_size, cube_size},
		{twice_cube_size + cube_size, cube_size}
	};

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

Skybox init_skybox(const char* const cubemap_path) {
	Skybox s;

	glGenBuffers(1, &s.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, s.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skybox_vertices), skybox_vertices, GL_STATIC_DRAW);

	s.shader = init_shader_program(skybox_vertex_shader, skybox_fragment_shader);
	s.texture = init_skybox_texture(cubemap_path);

	return s;
}

void deinit_skybox(const Skybox s) {
	glDeleteTextures(1, &s.texture);
	glDeleteProgram(s.shader);
	glDeleteBuffers(1, &s.vbo);
}

void draw_skybox(const Skybox s, const Camera* const camera) {
	glUseProgram(s.shader);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, s.texture);

	static byte first_call = 1;
	static GLint view_projection_id;
	if (first_call) {
		glUniform1i(glGetUniformLocation(s.shader, "texture_sampler"), 0);
		view_projection_id = glGetUniformLocation(s.shader, "view_projection");
		first_call = 0;
	}

	mat4 view_projection;
	memcpy(view_projection, camera -> view_projection, sizeof(mat4));

	/* This clears X, Y, and W. Z (depth) not cleared
	b/c it's always set to 1 in the vertex shader. */
	view_projection[3][0] = 0.0f;
	view_projection[3][1] = 0.0f;
	view_projection[3][3] = 0.0f;

	glUniformMatrix4fv(view_projection_id, 1, GL_FALSE, &view_projection[0][0]);

	glBindBuffer(GL_ARRAY_BUFFER, s.vbo);
	glVertexAttribPointer(0, 3, GL_BYTE, GL_FALSE, 0, NULL);

	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_FALSE);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);
}

#endif
