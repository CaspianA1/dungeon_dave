#include "rendering/entities/skybox.h"
#include "utils/shader.h"
#include "utils/texture.h"
#include "utils/buffer_defs.h"

/* TODO:
- Have a SkyboxRenderer interface that allows swapping out skybox textures
- Pixel art UV correction for skyboxes
*/

static GLuint init_skybox_texture(const GLchar* const cubemap_path) {
	SDL_Surface* const skybox_surface = init_surface(cubemap_path);
	const GLint skybox_w = skybox_surface -> w;

	////////// Failing if the dimensions are not right

	if (skybox_w != (skybox_surface -> h << 2) / 3)
		FAIL(CreateTexture, "The skybox with path '%s' does not have "
			"a width that equals 4/3 of its height", cubemap_path);

	//////////

	const GLint cube_size = skybox_w >> 2, twice_cube_size = skybox_w >> 1;
	const GLuint skybox_texture = preinit_texture(TexSkybox, TexNonRepeating, OPENGL_SCENE_MAG_FILTER, OPENGL_SCENE_MIN_FILTER, false);

	SDL_Surface* const face_surface = init_blank_surface(cube_size, cube_size);
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

			init_texture_data(TexSkybox, (GLsizei[]) {cube_size, i}, OPENGL_INPUT_PIXEL_FORMAT,
				OPENGL_DEFAULT_INTERNAL_PIXEL_FORMAT, OPENGL_COLOR_CHANNEL_TYPE, face_surface_pixels);
		}
	);

	glGenerateMipmap(TexSkybox);

	SDL_FreeSurface(face_surface);
	SDL_FreeSurface(skybox_surface);

	return skybox_texture;
}

static void update_uniforms(const Drawable* const drawable, const void* const param) {
	(void) param;
	ON_FIRST_CALL(use_texture(drawable -> diffuse_texture, drawable -> shader, "skybox_sampler", TexSkybox, TU_Skybox););
}

Skybox init_skybox(const GLchar* const cubemap_path) {
	return init_drawable_without_vertices(
		(uniform_updater_t) update_uniforms, GL_TRIANGLE_STRIP,
		init_shader(ASSET_PATH("shaders/skybox.vert"), NULL, ASSET_PATH("shaders/skybox.frag"), NULL),
		init_skybox_texture(cubemap_path)
	);
}

void draw_skybox(const Skybox* const skybox) {
	WITH_RENDER_STATE(glDepthFunc, GL_LEQUAL, GL_LESS, // Other depth testing mode for the skybox
		draw_drawable(*skybox, vertices_per_skybox, 0, NULL, UseShaderPipeline);
	);
}
