#include "rendering/entities/skybox.h"
#include "utils/texture.h" // For various texture creation utils
#include "utils/failure.h" // For `FAIL`
#include "cglm/cglm.h" // For various cglm defs
#include "data/constants.h" // For various constants
#include "utils/opengl_wrappers.h" // For various OpenGL wrappers
#include "utils/shader.h" // For `init_shader`
#include "utils/macro_utils.h" // For `ASSET_PATH`

/* TODO:
- Have a SkyboxRenderer interface that allows swapping out skybox textures
- Pixel art UV correction for skyboxes, instead of doing rescaling
- Figure out spherical caps
- A panorama -> skybox tool, where one can either cut off parts of the panorama,
	or insert the panorama as the middle of the skybox, and fill in outlines for the rest

Details on going from equiangular to equirectangular skyboxes:
	https://www.reddit.com/r/Unity3D/comments/6vfdpc/how_does_one_create_a_skybox_cubemap_from_scratch/
	https://vrkiwi.org/dev-blog/35-how-to-make-a-skybox-or-two/
	https://www.reddit.com/r/gamedev/comments/crxcu8/how_does_one_make_a_seamless_cube_shaped_skybox/
	https://plumetutorials.wordpress.com/2013/10/09/3d-tutorial-making-a-skybox/
	https://blog.google/products/google-ar-vr/bringing-pixels-front-and-center-vr-video/
	https://stackoverflow.com/questions/11504584/cubic-to-equirectangular-projection-algorithm
	https://en.wikipedia.org/wiki/Quadrilateralized_spherical_cube
	https://math.stackexchange.com/questions/118760/can-someone-please-explain-the-cube-to-sphere-mapping-formula-to-me
	The first comment for this video: https://www.youtube.com/watch?v=-ZutidNYVRE
	This video (uses Photoshop): https://www.youtube.com/watch?v=XZmr-XYRw3w

	The thumbnail to this: https://onlinelibrary.wiley.com/doi/10.1111/cgf.13843
	is this: https://onlinelibrary.wiley.com/cms/asset/a86e9106-591f-4d4c-9edb-2adde15de666/cgf13843-fig-0001-m.jpg
	Interesting image!

	Note:
		This article's technique http://mathproofs.blogspot.com/2005/07/mapping-cube-to-sphere.html
		is the same as this one: https://catlikecoding.com/unity/tutorials/procedural-meshes/cube-sphere/

	Also, radial projection may be the key
	The FFMPEG thing could possibly bring about something interesting as well
*/

//////////

typedef signed_byte sbvec3[3]; // `sb` for signed byte (TODO: use this type in the AO code)

/* Note: `ts` = triangle strip. See this link:
 * https://stackoverflow.com/questions/28375338/cube-using-single-gl-triangle-strip */
const sbvec3 skybox_vertices_ts[] = {
	{-1, 1, 1}, {1, 1, 1}, {-1, -1, 1}, {1, -1, 1},
	{1, -1, -1}, {1, 1, 1}, {1, 1, -1}, {-1, 1, 1}, {-1, 1, -1},
	{-1, -1, 1}, {-1, -1, -1}, {1, -1, -1}, {-1, 1, -1}, {1, 1, -1}
};

enum {vertices_per_skybox = ARRAY_LENGTH(skybox_vertices_ts)};

//////////

static GLuint init_skybox_texture(const GLchar* const texture_path, const GLfloat texture_scale) {
	SDL_Surface* const skybox_surface = init_surface(texture_path);

	////////// Failing if the dimensions are not right

	const GLint skybox_w = skybox_surface -> w;

	if (skybox_w != (skybox_surface -> h << 2) / 3)
		FAIL(CreateTexture, "The skybox with path '%s' does not have "
			"a width that equals 4/3 of its height", texture_path);

	//////////

	const GLuint skybox_texture = preinit_texture(TexSkybox, TexNonRepeating,
		OPENGL_SCENE_MAG_FILTER, OPENGL_SCENE_MIN_FILTER, false);

	const GLint face_size = skybox_w >> 2, twice_face_size = skybox_w >> 1;
	const GLint rescaled_face_size = (GLint) (face_size * texture_scale);

	SDL_Surface* const face_surface = init_blank_surface(rescaled_face_size, rescaled_face_size);

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

		SDL_BlitScaled(skybox_surface, &(SDL_Rect) {src_origin[0], src_origin[1],
			face_size, face_size}, face_surface, NULL);

		WITH_SURFACE_PIXEL_ACCESS(face_surface,
			// Flipping vertically for positive and negative y
			const bool flipping_vertically = (i == 2) || (i == 3);

			// Going by half of the cubemap size on the y axis if flipping vertically, and vice versa for x
			for (GLint y = 0; y < rescaled_face_size >> flipping_vertically; y++) {
				for (GLint x = 0; x < rescaled_face_size >> !flipping_vertically; x++) {
					sdl_pixel_t* const pixel = read_surface_pixel(face_surface, x, y);

					sdl_pixel_t* const to_swap = read_surface_pixel(face_surface,
						flipping_vertically ? x : (rescaled_face_size - x - 1),
						flipping_vertically ? (rescaled_face_size - y - 1) : y
					);

					const sdl_pixel_t temp = *pixel;
					*pixel = *to_swap;
					*to_swap = temp;
				}
			}

			init_texture_data(TexSkybox, (GLsizei[]) {rescaled_face_size, i}, OPENGL_INPUT_PIXEL_FORMAT,
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
	const GLuint
		skybox_texture = init_skybox_texture(config -> texture_path, config -> texture_scale),
		shader = init_shader(ASSET_PATH("shaders/skybox.vert"), NULL, ASSET_PATH("shaders/skybox.frag"), NULL);

	use_shader(shader);
	use_texture_in_shader(skybox_texture, shader, "skybox_sampler", TexSkybox, TU_Skybox);

	INIT_UNIFORM_VALUE(horizon_dist_scale, shader, 1f, config -> horizon_dist_scale);
	INIT_UNIFORM_VALUE(cylindrical_cap_blend_widths, shader, 2fv, 1,  config -> cylindrical_cap_blend_widths);

	return (Skybox) {
		init_drawable_with_vertices(define_vertex_spec, NULL, GL_STATIC_DRAW, GL_TRIANGLE_STRIP,
		(List) {.data = (void*) skybox_vertices_ts, .item_size = sizeof(skybox_vertices_ts[0]), .length = vertices_per_skybox},
		shader, skybox_texture, 0)
	};
}

void deinit_skybox(const Skybox* const skybox) {
	deinit_drawable(skybox -> drawable);
}

void draw_skybox(const Skybox* const skybox) {
	WITH_RENDER_STATE(glDepthFunc, GL_EQUAL, GL_LESS, // Other depth testing mode for the skybox
		draw_drawable(skybox -> drawable, vertices_per_skybox, 0, NULL, UseShaderPipeline | UseVertexSpec);
	);
}
