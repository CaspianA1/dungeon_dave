#ifndef SKYBOX_C
#define SKYBOX_C

#include "headers/skybox.h"
#include "headers/shader.h"
#include "headers/texture.h"
#include "headers/buffer_defs.h"

// TODO: make a SkyboxRenderer interface, instead of having unique vbos, vaos, and shaders for each skybox

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
	const GLuint skybox_texture = preinit_texture(TexSkybox, TexNonRepeating, OPENGL_SCENE_MAG_FILTER, OPENGL_SCENE_MIN_FILTER, false);

	SDL_Surface* const face_surface = init_blank_surface(cube_size, cube_size, SDL_PIXEL_FORMAT);
	void* const face_surface_pixels = face_surface -> pixels;

	// Right, left, top, bottom, back, front
	const ivec2 src_origins[faces_per_cubemap] = {
		{twice_cube_size, cube_size},
		{0, cube_size},
		{cube_size, 0},
		{cube_size, twice_cube_size},
		{cube_size, cube_size},
		{twice_cube_size + cube_size, cube_size}
	};

	WITH_SURFACE_PIXEL_ACCESS(face_surface,
		for (byte i = 0; i < faces_per_cubemap; i++) {
			const GLint* const src_origin = src_origins[i];

			SDL_BlitSurface(skybox_surface, &(SDL_Rect) {src_origin[0], src_origin[1], cube_size, cube_size}, face_surface, NULL);

			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, OPENGL_DEFAULT_INTERNAL_PIXEL_FORMAT,
				cube_size, cube_size, 0, OPENGL_INPUT_PIXEL_FORMAT, OPENGL_COLOR_CHANNEL_TYPE, face_surface_pixels);
		}
	);

	glGenerateMipmap(TexSkybox);

	deinit_surface(face_surface);
	deinit_surface(skybox_surface);

	return skybox_texture;
}

static void define_vertex_spec(void) {
	define_vertex_spec_index(false, false, 0, vertices_per_triangle, 0, 0, GL_BYTE);
}

static void update_uniforms(const Drawable* const drawable, const void* const param) {
	const GLuint shader = drawable -> shader;

	static GLint view_projection_id;

	ON_FIRST_CALL(
		INIT_UNIFORM(view_projection, shader);
		use_texture(drawable -> diffuse_texture, shader, "skybox_sampler", TexSkybox, TU_Skybox);
	);

	mat4 view_projection;
	glm_mat4_copy((vec4*) param, view_projection);
	glm_vec4_zero(view_projection[3]); // This clears x, y, z, and w

	UPDATE_UNIFORM(view_projection, Matrix4fv, 1, GL_FALSE, &view_projection[0][0]);
}

Skybox init_skybox(const GLchar* const cubemap_path, const GLfloat texture_rescale_factor) {
	const buffer_size_t num_vertices = ARRAY_LENGTH(skybox_vertices);

	return init_drawable_with_vertices(define_vertex_spec,
		(uniform_updater_t) update_uniforms, GL_STATIC_DRAW, GL_TRIANGLE_STRIP,
		(List) {(void*) skybox_vertices, sizeof(GLbyte[vertices_per_triangle]), num_vertices, num_vertices},

		init_shader(ASSET_PATH("shaders/skybox.vert"), NULL, ASSET_PATH("shaders/skybox.frag")),
		init_skybox_texture(cubemap_path, texture_rescale_factor)
	);
}

void draw_skybox(const Skybox* const skybox, const mat4 view_projection) {
	WITH_RENDER_STATE(glDepthFunc, GL_LEQUAL, GL_LESS, // Other depth testing mode for the skybox
		draw_drawable(*skybox, ARRAY_LENGTH(skybox_vertices), view_projection, UseShaderPipeline | BindVertexBufferAndSpec);
	);
}

#endif
