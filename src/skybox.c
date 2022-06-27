#ifndef SKYBOX_C
#define SKYBOX_C

#include "headers/skybox.h"
#include "headers/shader.h"
#include "headers/texture.h"
#include "headers/buffer_defs.h"

// https://stackoverflow.com/questions/28375338/cube-using-single-gl-triangle-strip
static const GLbyte skybox_vertices[][vertices_per_triangle] = {
	{-1, 1, 1}, {1, 1, 1}, {-1, -1, 1}, {1, -1, 1},
	{1, -1, -1}, {1, 1, 1}, {1, 1, -1}, {-1, 1, 1}, {-1, 1, -1},
	{-1, -1, 1}, {-1, -1, -1}, {1, -1, -1}, {-1, 1, -1}, {1, 1, -1}
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
	const GLuint skybox_texture = preinit_texture(TexSkybox, TexNonRepeating, OPENGL_SCENE_MAG_FILTER, OPENGL_SKYBOX_MIN_FILTER, false);

	SDL_Surface* const face_surface = init_blank_surface(cube_size, cube_size, SDL_PIXEL_FORMAT);

	typedef struct {const GLint x, y;} ivec2;

	// Right, left, top, bottom, back, front
	const ivec2 src_origins[faces_per_cubemap] = {
		{twice_cube_size, cube_size},
		{0, cube_size},
		{cube_size, 0},
		{cube_size, twice_cube_size},
		{cube_size, cube_size},
		{twice_cube_size + cube_size, cube_size}
	};

	for (byte i = 0; i < faces_per_cubemap; i++) {
		const ivec2 src_origin = src_origins[i];

		SDL_BlitSurface(skybox_surface, &(SDL_Rect) {src_origin.x, src_origin.y, cube_size, cube_size}, face_surface, NULL);
		write_surface_to_texture(face_surface, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, OPENGL_DEFAULT_INTERNAL_PIXEL_FORMAT);
	}

	glGenerateMipmap(TexSkybox);

	deinit_surface(face_surface);
	deinit_surface(skybox_surface);

	return skybox_texture;
}

static void define_vertex_spec_for_skybox(void) {
	define_vertex_spec_index(false, false, 0, vertices_per_triangle, 0, 0, GL_BYTE);
}

static void update_uniforms(const Drawable* const drawable, const void* const param) {
	const GLuint shader = drawable -> shader;

	static GLint view_projection_id;

	ON_FIRST_CALL(
		INIT_UNIFORM(view_projection, shader);
		use_texture(drawable -> diffuse_texture, shader, "texture_sampler", TexSkybox, SKYBOX_TEXTURE_UNIT);
	);

	mat4 view_projection;
	glm_mat4_copy((vec4*) param, view_projection);

	/* This clears X, Y, and W. Z (depth) not cleared
	b/c it's always set to 1 in the vertex shader. */
	view_projection[3][0] = 0.0f;
	view_projection[3][1] = 0.0f;
	view_projection[3][3] = 0.0f;

	UPDATE_UNIFORM(view_projection, Matrix4fv, 1, GL_FALSE, &view_projection[0][0]);
}

Skybox init_skybox(const GLchar* const cubemap_path, const GLfloat texture_rescale_factor) {
	/* TODO: when creating a new skybox, just switch out the texture,
	instead of recreating the vertex buffer, spec, and shader. */

	const buffer_size_t num_vertices = ARRAY_LENGTH(skybox_vertices);

	return init_drawable(define_vertex_spec_for_skybox,
		(uniform_updater_t) update_uniforms, GL_STATIC_DRAW, GL_TRIANGLE_STRIP,
		(List) {(void*) skybox_vertices, sizeof(GLbyte[vertices_per_triangle]), num_vertices, num_vertices},

		init_shader(ASSET_PATH("shaders/skybox.vert"), NULL, ASSET_PATH("shaders/skybox.frag")),
		init_skybox_texture(cubemap_path, texture_rescale_factor)
	);
}

void draw_skybox(const Skybox* const skybox, const mat4 view_projection) {
	WITH_RENDER_STATE(glDepthFunc, GL_LEQUAL, GL_LESS,
		WITH_RENDER_STATE(glDepthMask, GL_FALSE, GL_TRUE,
			draw_drawable(*skybox, ARRAY_LENGTH(skybox_vertices), view_projection);
		);
	);
}

#endif
