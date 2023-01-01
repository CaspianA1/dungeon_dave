#include "rendering/entities/skybox.h"
#include "utils/texture.h" // For various texture creation utils
#include "utils/failure.h" // For `FAIL`
#include "cglm/cglm.h" // For `ivec2`
#include "data/constants.h" // For `faces_per_cubemap`, and `vertices_per_skybox`
#include "utils/opengl_wrappers.h" // For various OpenGL wrappers
#include "utils/shader.h" // For `init_shader`
#include "utils/macro_utils.h" // For `ASSET_PATH`

/* TODO:
- Have a SkyboxRenderer interface that allows swapping out skybox textures
- Pixel art UV correction for skyboxes
- A panorama -> skybox tool, where one can either cut off parts of the panorama,
	or insert the panorama as the middle of the skybox, and fill in outlines for the rest
*/

static GLuint init_skybox_texture(const GLchar* const texture_path) {
	SDL_Surface* const skybox_surface = init_surface(texture_path);

	const GLint skybox_w = skybox_surface -> w;

	////////// Failing if the dimensions are not right

	if (skybox_w != (skybox_surface -> h << 2) / 3)
		FAIL(CreateTexture, "The skybox with path '%s' does not have "
			"a width that equals 4/3 of its height", texture_path);

	//////////

	const GLint cube_size = skybox_w >> 2, twice_cube_size = skybox_w >> 1;
	const GLuint skybox_texture = preinit_texture(TexSkybox, TexNonRepeating, OPENGL_SCENE_MAG_FILTER, OPENGL_SCENE_MIN_FILTER, false);

	SDL_Surface* const face_surface = init_blank_surface(cube_size, cube_size);
	void* const face_surface_pixels = face_surface -> pixels;

	// Order: pos x, neg x, pos y, neg y, pos z, neg z
	const ivec2 src_origins[faces_per_cubemap] = {
		{twice_cube_size, cube_size},
		{0, cube_size},
		{cube_size, 0},
		{cube_size, twice_cube_size},
		{twice_cube_size + cube_size, cube_size},
		{cube_size, cube_size}
	};

	for (byte i = 0; i < faces_per_cubemap; i++) {
		const GLint* const src_origin = src_origins[i];

		SDL_BlitSurface(skybox_surface, &(SDL_Rect) {src_origin[0], src_origin[1],
			cube_size, cube_size}, face_surface, NULL);

		WITH_SURFACE_PIXEL_ACCESS(face_surface,
			// Flipping vertically for positive and negative y
			const bool flipping_vertically = (i == 2) || (i == 3);

			// Going by half of the cubemap size on the y axis if flipping vertically, and vice versa for x
			for (GLint y = 0; y < cube_size >> flipping_vertically; y++) {
				for (GLint x = 0; x < cube_size >> !flipping_vertically; x++) {
					sdl_pixel_t* const pixel = read_surface_pixel(face_surface, x, y);

					sdl_pixel_t* const to_swap = read_surface_pixel(face_surface,
						flipping_vertically ? x : (cube_size - x - 1),
						flipping_vertically ? (cube_size - y - 1) : y
					);

					const sdl_pixel_t temp = *pixel;
					*pixel = *to_swap;
					*to_swap = temp;
				}
			}

			init_texture_data(TexSkybox, (GLsizei[]) {cube_size, i}, OPENGL_INPUT_PIXEL_FORMAT,
				OPENGL_DEFAULT_INTERNAL_PIXEL_FORMAT, OPENGL_COLOR_CHANNEL_TYPE, face_surface_pixels);
		);
	}

	init_texture_mipmap(TexSkybox);

	deinit_surface(face_surface);
	deinit_surface(skybox_surface);

	return skybox_texture;
}

Skybox init_skybox(const SkyboxConfig* const config) {
	const GLuint
		shader = init_shader(ASSET_PATH("shaders/skybox.vert"), NULL, ASSET_PATH("shaders/skybox.frag"), NULL),
		albedo_texture = init_skybox_texture(config -> texture_path);

	use_shader(shader);
	use_texture_in_shader(albedo_texture, shader, "skybox_sampler", TexSkybox, TU_Skybox);
	INIT_UNIFORM_VALUE(map_cube_to_sphere, shader, 1ui, config -> map_cube_to_sphere);

	return init_drawable_without_vertices(NULL, GL_TRIANGLE_STRIP, shader, albedo_texture, 0);
}

void draw_skybox(const Skybox* const skybox) {
	WITH_RENDER_STATE(glDepthFunc, GL_LEQUAL, GL_LESS, // Other depth testing mode for the skybox
		draw_drawable(*skybox, vertices_per_skybox, 0, NULL, UseShaderPipeline);
	);
}
