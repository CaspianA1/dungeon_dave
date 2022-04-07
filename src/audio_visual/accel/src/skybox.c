#ifndef SKYBOX_C
#define SKYBOX_C

#include "headers/skybox.h"
#include "data/shaders.c"
#include "texture.c"

static const GLbyte skybox_vertices[] = {
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

static GLuint init_skybox_texture(const GLchar* const path) {
	SDL_Surface* const skybox_surface = init_surface(path);

	const GLint skybox_w = skybox_surface -> w;
	const GLint cube_size = skybox_w >> 2, twice_cube_size = skybox_w >> 1;
	const GLuint skybox = preinit_texture(TexSkybox, TexNonRepeating, OPENGL_SCENE_MAG_FILTER, OPENGL_SKYBOX_MIN_FILTER);

	SDL_Surface* const face_surface = init_blank_surface(cube_size, cube_size, SDL_PIXEL_FORMAT);

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

		SDL_BlitSurface(skybox_surface, &src_rect, face_surface, NULL);
		write_surface_to_texture(face_surface, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, OPENGL_DEFAULT_INTERNAL_PIXEL_FORMAT);
	}

	glGenerateMipmap(TexSkybox);

	deinit_surface(face_surface);
	deinit_surface(skybox_surface);

	return skybox;
}

Skybox init_skybox(const GLchar* const cubemap_path) {
	static bool first_call = true;

	if (first_call) {
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
		first_call = false;
	}

	const GLuint vbo = init_gpu_buffer();
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skybox_vertices), skybox_vertices, GL_STATIC_DRAW);

	return (Skybox) {
		.vbo = vbo,
		.shader = init_shader(skybox_vertex_shader, skybox_fragment_shader),
		.texture = init_skybox_texture(cubemap_path)
	};
}

void deinit_skybox(const Skybox s) {
	deinit_texture(s.texture);
	deinit_shader(s.shader);
	glDeleteBuffers(1, &s.vbo);
}

void draw_skybox(const Skybox s, const Camera* const camera) {
	use_shader(s.shader);

	static GLint model_view_projection_id;
	static bool first_call = true;

	if (first_call) {
		INIT_UNIFORM(model_view_projection, s.shader);
		use_texture(s.texture, s.shader, "texture_sampler", TexSkybox, SKYBOX_TEXTURE_UNIT);
		first_call = false;
	}

	mat4 model_view_projection; // TODO: use glm_mat4_copy
	memcpy(model_view_projection, camera -> model_view_projection, sizeof(mat4));

	/* This clears X, Y, and W. Z (depth) not cleared
	b/c it's always set to 1 in the vertex shader. */
	model_view_projection[3][0] = 0.0f;
	model_view_projection[3][1] = 0.0f;
	model_view_projection[3][3] = 0.0f;

	UPDATE_UNIFORM(model_view_projection, Matrix4fv, 1, GL_FALSE, &model_view_projection[0][0]);

	glBindBuffer(GL_ARRAY_BUFFER, s.vbo);

	WITH_VERTEX_ATTRIBUTE(false, 0, 3, GL_BYTE, 0, 0,
		WITH_RENDER_STATE(glDepthFunc, GL_LEQUAL, GL_LESS,
			WITH_RENDER_STATE(glDepthMask, GL_FALSE, GL_TRUE,
				glDrawArrays(GL_TRIANGLES, 0, 36);
			);
		);
	);
}

#endif
