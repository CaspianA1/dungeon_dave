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

// https://stackoverflow.com/questions/28375338/cube-using-single-gl-triangle-strip
const signed_byte vertices[][3] = {
	{-1, 1, 1}, {1, 1, 1}, {-1, -1, 1}, {1, -1, 1},
	{1, -1, -1}, {1, 1, 1}, {1, 1, -1}, {-1, 1, 1}, {-1, 1, -1},
	{-1, -1, 1}, {-1, -1, -1}, {1, -1, -1}, {-1, 1, -1}, {1, 1, -1}
};

static GLuint init_skybox_texture(const GLchar* const texture_path) {
	SDL_Surface* const skybox_surface = init_surface(texture_path);

	////////// Failing if the dimensions are not right

	const GLint skybox_w = skybox_surface -> w;

	if (skybox_w != (skybox_surface -> h << 2) / 3)
		FAIL(CreateTexture, "The skybox with path '%s' does not have "
			"a width that equals 4/3 of its height", texture_path);

	//////////

	const GLint face_size = skybox_w >> 2, twice_face_size = skybox_w >> 1;
	const GLuint skybox_texture = preinit_texture(TexSkybox, TexNonRepeating, TexLinear, TexLinearMipmapped, false);

	SDL_Surface* const face_surface = init_blank_surface(face_size, face_size);

	// Order: pos x, neg x, pos y, neg y, pos z, neg z
	const ivec2 src_origins[faces_per_cubemap] = {
		{twice_face_size, face_size},
		{0, face_size},
		{face_size, 0},
		{face_size, twice_face_size},
		{twice_face_size + face_size, face_size},
		{face_size, face_size}
	};

	for (byte i = 0; i < faces_per_cubemap; i++) {
		const GLint* const src_origin = src_origins[i];

		SDL_BlitSurface(skybox_surface, &(SDL_Rect) {src_origin[0], src_origin[1],
			face_size, face_size}, face_surface, NULL);

		WITH_SURFACE_PIXEL_ACCESS(face_surface,
			// Flipping vertically for positive and negative y
			const bool flipping_vertically = (i == 2) || (i == 3);

			// Going by half of the cubemap size on the y axis if flipping vertically, and vice versa for x
			for (GLint y = 0; y < face_size >> flipping_vertically; y++) {
				for (GLint x = 0; x < face_size >> !flipping_vertically; x++) {
					sdl_pixel_t* const pixel = read_surface_pixel(face_surface, x, y);

					sdl_pixel_t* const to_swap = read_surface_pixel(face_surface,
						flipping_vertically ? x : (face_size - x - 1),
						flipping_vertically ? (face_size - y - 1) : y
					);

					const sdl_pixel_t temp = *pixel;
					*pixel = *to_swap;
					*to_swap = temp;
				}
			}

			//////////

			init_texture_data(TexSkybox, (GLsizei[]) {face_size, i}, OPENGL_INPUT_PIXEL_FORMAT,
				OPENGL_DEFAULT_INTERNAL_PIXEL_FORMAT, OPENGL_COLOR_CHANNEL_TYPE, face_surface -> pixels);
		);
	}

	init_texture_mipmap(TexSkybox);

	deinit_surface(face_surface);
	deinit_surface(skybox_surface);

	return skybox_texture;
}

static void define_vertex_spec(void) {
	define_vertex_spec_index(false, false, 0, 3, 0, 0, GL_BYTE);
}

Skybox init_skybox(const SkyboxConfig* const config) {
	const GLuint shader = init_shader(ASSET_PATH("shaders/skybox.vert"), NULL, ASSET_PATH("shaders/skybox.frag"), NULL);
	const GLuint albedo_texture = init_skybox_texture(config -> texture_path);

	use_shader(shader);
	use_texture_in_shader(albedo_texture, shader, "skybox_sampler", TexSkybox, TU_Skybox);
	INIT_UNIFORM_VALUE(map_cube_to_sphere, shader, 1ui, config -> map_cube_to_sphere);

	const Drawable drawable = init_drawable_with_vertices(define_vertex_spec, NULL, GL_STATIC_DRAW, GL_TRIANGLE_STRIP,
		(List) {.data = (void*) vertices, .item_size = sizeof(vertices[0]), .length = ARRAY_LENGTH(vertices)},
		shader, albedo_texture, 0
	);

	return drawable;
}

void draw_skybox(const Skybox* const skybox) {
	WITH_RENDER_STATE(glDepthFunc, GL_LEQUAL, GL_LESS, // Other depth testing mode for the skybox
		draw_drawable(*skybox, ARRAY_LENGTH(vertices), 0, NULL, UseShaderPipeline | BindVertexSpec);
	);
}
