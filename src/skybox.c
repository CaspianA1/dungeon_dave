#ifndef SKYBOX_C
#define SKYBOX_C

#include "headers/skybox.h"
#include "headers/shader_code.h"

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

static GLuint init_skybox_texture(const GLchar* const cubemap_path, const GLfloat texture_rescale_factor) {
	SDL_Surface* skybox_surface = init_surface(cubemap_path);

	////////// Rescaling the skybox if needed

	if (texture_rescale_factor != 1.0f) {
		SDL_Surface* const rescaled_skybox_surface = init_blank_surface(
			(GLsizei) (skybox_surface -> w * texture_rescale_factor),
			(GLsizei) (skybox_surface -> h * texture_rescale_factor),
			SDL_PIXEL_FORMAT);

		SDL_BlitScaled(skybox_surface, NULL, rescaled_skybox_surface, NULL);
		deinit_surface(skybox_surface);

		skybox_surface = rescaled_skybox_surface;
	}

	const GLint skybox_w = skybox_surface -> w;

	////////// Failing if the dimensions are not right

	if (skybox_w != (skybox_surface -> h << 2) / 3)
		FAIL(CreateSkybox, "The skybox with path '%s' does not have "
			"a width that equals 4/3 of its height", cubemap_path);

	//////////

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

Skybox init_skybox(const GLchar* const cubemap_path, const GLfloat texture_rescale_factor) {
	const GLuint vertex_buffer = init_gpu_buffer(), vertex_spec = init_vertex_spec();
	use_vertex_buffer(vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skybox_vertices), skybox_vertices, GL_STATIC_DRAW);

	use_vertex_spec(vertex_spec);
	define_vertex_spec_index(false, false, 0, 3, 0, 0, GL_BYTE);

	/* TODO: share this vertex buffer, vertex spec, and shader
	between different skyboxes. Perhaps a SkyboxRenderer struct? */

	return (Skybox) {
		.vertex_buffer = vertex_buffer, .vertex_spec = vertex_spec,
		.shader = init_shader(skybox_vertex_shader, skybox_fragment_shader),
		.texture = init_skybox_texture(cubemap_path, texture_rescale_factor)
	};
}

void deinit_skybox(const Skybox s) {
	deinit_gpu_buffer(s.vertex_buffer);
	deinit_vertex_spec(s.vertex_spec);
	deinit_texture(s.texture);
	deinit_shader(s.shader);
}

void draw_skybox(const Skybox s, const Camera* const camera) {
	use_shader(s.shader);

	static GLint model_view_projection_id;

	ON_FIRST_CALL(
		INIT_UNIFORM(model_view_projection, s.shader);
		use_texture(s.texture, s.shader, "texture_sampler", TexSkybox, SKYBOX_TEXTURE_UNIT);
	);

	mat4 model_view_projection;
	glm_mat4_copy((vec4*) camera -> model_view_projection, model_view_projection);

	/* This clears X, Y, and W. Z (depth) not cleared
	b/c it's always set to 1 in the vertex shader. */
	model_view_projection[3][0] = 0.0f;
	model_view_projection[3][1] = 0.0f;
	model_view_projection[3][3] = 0.0f;

	UPDATE_UNIFORM(model_view_projection, Matrix4fv, 1, GL_FALSE, &model_view_projection[0][0]);

	use_vertex_buffer(s.vertex_buffer);
	use_vertex_spec(s.vertex_spec);

	WITH_RENDER_STATE(glDepthFunc, GL_LEQUAL, GL_LESS,
		WITH_RENDER_STATE(glDepthMask, GL_FALSE, GL_TRUE,
			glDrawArrays(GL_TRIANGLES, 0, 36);
		);
	);
}

#endif
