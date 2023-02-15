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
- Pixel art UV correction for skyboxes
- A panorama -> skybox tool, where one can either cut off parts of the panorama,
	or insert the panorama as the middle of the skybox, and fill in outlines for the rest

Details on going from equiangular to equirectangular skyboxes:
	https://www.reddit.com/r/Unity3D/comments/6vfdpc/how_does_one_create_a_skybox_cubemap_from_scratch/
	https://vrkiwi.org/dev-blog/35-how-to-make-a-skybox-or-two/
	https://www.reddit.com/r/gamedev/comments/crxcu8/how_does_one_make_a_seamless_cube_shaped_skybox/
	https://plumetutorials.wordpress.com/2013/10/09/3d-tutorial-making-a-skybox/
	https://blog.google/products/google-ar-vr/bringing-pixels-front-and-center-vr-video/
	https://stackoverflow.com/questions/11504584/cubic-to-equirectangular-projection-algorithm
	The first comment for this video: https://www.youtube.com/watch?v=-ZutidNYVRE
	This video (uses Photoshop): https://www.youtube.com/watch?v=XZmr-XYRw3w

	The thumbnail to this: https://onlinelibrary.wiley.com/doi/10.1111/cgf.13843
	is this: https://onlinelibrary.wiley.com/cms/asset/a86e9106-591f-4d4c-9edb-2adde15de666/cgf13843-fig-0001-m.jpg
	Interesting image!

	Note:
		This article's technique http://mathproofs.blogspot.com/2005/07/mapping-cube-to-sphere.html
		is the same as this one: https://catlikecoding.com/unity/tutorials/procedural-meshes/cube-sphere/

	Also, radial projection may be the key
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

static GLuint init_skybox_texture(const GLchar* const texture_path, GLint* const face_size_ref, const bool map_cube_to_sphere) {
	SDL_Surface* const skybox_surface = init_surface(texture_path);

	////////// Failing if the dimensions are not right

	const GLint skybox_w = skybox_surface -> w;

	if (skybox_w != (skybox_surface -> h << 2) / 3)
		FAIL(CreateTexture, "The skybox with path '%s' does not have "
			"a width that equals 4/3 of its height", texture_path);

	//////////

	const GLuint skybox_texture = preinit_texture(TexSkybox, TexNonRepeating,
		map_cube_to_sphere ? TexNearest : OPENGL_SCENE_MAG_FILTER,
		OPENGL_SCENE_MIN_FILTER, false);

	const GLint face_size = skybox_w >> 2, twice_face_size = skybox_w >> 1;
	*face_size_ref = face_size;

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

static void define_skybox_vertex_spec(void) {
	define_vertex_spec_index(false, false, 0, 3, 0, 0, GL_BYTE);
}

static void define_sphere_mesh_vertex_spec(void) {
	define_vertex_spec_index(false, true, 0, 3, 0, 0, GL_FLOAT);
}

////////// Sphere code

static void make_matrices_for_skybox_predistortion(const GLfloat y_weight, const byte level_size[3], const vec3 scale_ratios,
	mat4 translation_scaling_matrix, mat4 rotation_matrices[faces_per_cubemap], mat4 view_projection_matrices[faces_per_cubemap]) {

	/* Possible matrix format conversions:
	- Tanslation-scaling matrix to a `mat4x3`
	- Rotation matrices to a series of `mat3x3`s
	- View-projections to a series of `mat3x4`s */

	////////// Making the translation-scaling matrix

	const GLfloat half_level_extent_across = glm_vec3_norm((vec3) {level_size[0], level_size[1], level_size[2]}) * 0.5f;

	vec3 scaling_vector;
	glm_vec3_scale((GLfloat*) scale_ratios, half_level_extent_across, scaling_vector);

	const GLfloat offset_y = glm_lerp(scaling_vector[1], -scaling_vector[1], y_weight);

	glm_mat4_identity(translation_scaling_matrix);
	glm_translate_y(translation_scaling_matrix, offset_y);
	glm_scale(translation_scaling_matrix, scaling_vector);

	////////// Making the rotation matrices

	const GLfloat cube_angle = GLM_PI_2f;

	for (byte i = 0; i < faces_per_cubemap; i++) glm_mat4_identity(rotation_matrices[i]);

	#define APPLY_CUBE_ROTATION(axis, sign, src_index, dest_index) glm_rotate_##axis(\
		rotation_matrices[src_index], sign cube_angle, rotation_matrices[dest_index])

	APPLY_CUBE_ROTATION(x, +, 1, 1);
	APPLY_CUBE_ROTATION(z, -, 2, 2);
	APPLY_CUBE_ROTATION(x, -, 3, 3);
	APPLY_CUBE_ROTATION(z, +, 4, 4);
	APPLY_CUBE_ROTATION(z, +, 4, 5);

	#undef APPLY_CUBE_ROTATION

	////////// Making the view-projection matrices

	const GLfloat
		cube_aspect_ratio = 1.0f,
		cube_near_clip_dist = constants.camera.near_clip_dist, // Any far clip dist works
		cube_far_clip_dist = constants.camera.near_clip_dist + GLM_FLT_EPSILON;

	// TODO: can I infer the up dirs?
	const sbvec3 // Here, the directions match OpenGL's cubemap order of pos/neg x, pos/neg y, pos/neg z
		forward_dirs[faces_per_cubemap] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}},
		up_dirs[faces_per_cubemap] = {{0, -1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}, {0, -1, 0}, {0, -1, 0}};

	mat4 projection;
	glm_perspective(cube_angle, cube_aspect_ratio, cube_near_clip_dist, cube_far_clip_dist, projection);

	for (byte i = 0; i < faces_per_cubemap; i++) {
		const signed_byte *const forward_dir = forward_dirs[i], *const up_dir = up_dirs[i];

		mat4 view;

		glm_look(
			(vec3) {0, 0, 0},
			(vec3) {forward_dir[0], forward_dir[1], forward_dir[2]},
			(vec3) {up_dir[0], up_dir[1], up_dir[2]},
			view
		);

		glm_mul(projection, view, view_projection_matrices[i]);
	}

	//////////
}

static GLuint make_spherically_distorted_skybox_texture(const GLuint orig_texture,
	const GLint face_size, const SkyboxSphericalDistortionConfig config) {

	// TODO: do optimizations like indexing, a triangle strip, and/or more (no repeating vertices will cover all unrazerized areas)

	/* How all of this works:

	1. A subdivided flat face, ranging from -1 to 1 on x and z, and a constant 1 on y, is created.
	2. Next, based on the level size, scale ratios, and a y weight (from 0 to 1), a translation-scaling matrix is made.

	3. 6 instances of the subdivided plane are drawn.
	4. In the vertex shader, each subdivided vertex is first warped into a sphere vertex.
	5. The sphere vertex is rotated to fit a side of a sphere. The rotation matrix index depends on the instance id. That output equals `gl_Position`.
	6. The vertex shader also outputs a translated, scaled, and rotated sphere vertex (in world space).

	7. In the geometry shader, 6 layered rendering instances are done to draw the vertices to the 6 sides of a cubemap.
	8. The geometry shader passes on the rotated sphere vertex as a model-space vetex to use to sample from the cubemap with.
	9. It also multiplies the world-space sphere vertex pos with the view-projection matrix corresponding to the current cubemap layer to determine the correct fragment pos.

	10. And finally, the fragment shader uses the rotated model-space position to sample from the original cubemap. */

	////////// Validating the scale ratios

	for (byte i = 0; i < ARRAY_LENGTH(config.scale_ratios); i++) {
		const GLfloat min_sub_ratio = 1.0f;

		// TODO: print a letter component here
		if (config.scale_ratios[i] < min_sub_ratio) FAIL(CreateTexture,
			"Cannot create a spherical skybox texture "
			"because a scale ratio is below %g", (GLdouble) min_sub_ratio);
	}

	////////// Face subdivision

	const byte fineness = constants.skybox_sphere_fineness;
	List subdivided_face = init_list(vertices_per_face * fineness * fineness, vec3);

	const GLfloat yf = 1.0f, unit_size = 1.0f / fineness;
	const GLfloat twice_unit_size = unit_size * 2.0f;

	for (byte z = 0; z < fineness; z++) {
		const GLfloat zf = z * twice_unit_size - 1.0f;

		for (byte x = 0; x < fineness; x++) {
			const GLfloat xf = x * twice_unit_size - 1.0f;

			//////////

			#define A {xf, yf, zf}
			#define B {xf, yf, zf + twice_unit_size}
			#define C {xf + twice_unit_size, yf, zf + twice_unit_size}
			#define D {xf + twice_unit_size, yf, zf}

			const vec3 sub_face_triangles[vertices_per_face] = {B, A, C, C, A, D};
			push_array_to_list(&subdivided_face, sub_face_triangles, ARRAY_LENGTH(sub_face_triangles));

			#undef A
			#undef B
			#undef C
			#undef D
		}
	}

	////////// Prerendering setup

	// Making a `Drawable` for prerendering (TODO: try using MSAA)
	const Drawable cube_to_sphere_drawable = init_drawable_with_vertices(
		define_sphere_mesh_vertex_spec, NULL, GL_STATIC_DRAW, GL_TRIANGLES, subdivided_face,

		init_shader(
			ASSET_PATH("shaders/convert_equiangular_skybox.vert"),
			ASSET_PATH("shaders/convert_equiangular_skybox.geom"),
			ASSET_PATH("shaders/convert_equiangular_skybox.frag"), NULL
		),

		0, 0
	);

	const GLuint // TODO: use anisotropic filtering, or not?
		warped_texture = preinit_texture(TexSkybox, TexNonRepeating, OPENGL_SCENE_MAG_FILTER, OPENGL_SCENE_MIN_FILTER, true),
		framebuffer = init_framebuffer();

	use_framebuffer(framebuffer_target, framebuffer);

	const GLenum attachment_point = GL_COLOR_ATTACHMENT0;
	const GLint upscaled_face_size = (GLint) (face_size * config.output_texture_scale);

	for (byte i = 0; i < faces_per_cubemap; i++)
		init_texture_data(TexSkybox, (GLsizei[]) {upscaled_face_size, i}, OPENGL_INPUT_PIXEL_FORMAT,
			OPENGL_DEFAULT_INTERNAL_PIXEL_FORMAT, OPENGL_COLOR_CHANNEL_TYPE, NULL);

	glFramebufferTexture(framebuffer_target, attachment_point, warped_texture, 0);
	glDrawBuffer(attachment_point); glReadBuffer(GL_NONE);
	check_framebuffer_completeness();

	////////// Initializing shader uniforms

	mat4 translation_scaling_matrix, rotation_matrices[faces_per_cubemap], view_projections[faces_per_cubemap];

	make_matrices_for_skybox_predistortion(config.percentage_towards_y_bottom,
		config.level_size, config.scale_ratios, translation_scaling_matrix, rotation_matrices, view_projections);

	const GLuint shader = cube_to_sphere_drawable.shader;

	use_shader(shader); // Binding the orig texture to the shader
	use_texture_in_shader(orig_texture, shader, "equiangular_skybox_sampler", TexSkybox, TU_Skybox);
	INIT_UNIFORM_VALUE(translation_scaling_matrix, shader, Matrix4fv, 1, GL_FALSE, (const GLfloat*) translation_scaling_matrix);
	INIT_UNIFORM_VALUE(rotation_matrices, shader, Matrix4fv, faces_per_cubemap, GL_FALSE, (const GLfloat*) rotation_matrices);
	INIT_UNIFORM_VALUE(view_projections, shader, Matrix4fv, faces_per_cubemap, GL_FALSE, (const GLfloat*) view_projections);

	////////// Drawing

	GLint prev_viewport_bounds[4];
	glGetIntegerv(GL_VIEWPORT, prev_viewport_bounds);

	glViewport(0, 0, upscaled_face_size, upscaled_face_size);
		draw_drawable(cube_to_sphere_drawable, subdivided_face.length, faces_per_cubemap, NULL, BindVertexSpec);
		use_framebuffer(framebuffer_target, 0);
	glViewport(prev_viewport_bounds[0], prev_viewport_bounds[1], prev_viewport_bounds[2], prev_viewport_bounds[3]);

	////////// Prerendering deinit

	deinit_framebuffer(framebuffer);
	deinit_drawable(cube_to_sphere_drawable);
	deinit_list(subdivided_face);

	////////// Making a mipmap, and returning the warped texture

	// TODO: perhaps avoid this binding somehow?
	use_texture(TexSkybox, warped_texture);
	init_texture_mipmap(TexSkybox);

	return warped_texture;
}

//////////

Skybox init_skybox(const SkyboxConfig* const config) {
	const SkyboxSphericalDistortionConfig* const spherical_distortion_config = config -> spherical_distortion_config;
	const bool map_cube_to_sphere = spherical_distortion_config != NULL;

	GLint face_size;
	const GLuint plain_texture = init_skybox_texture(config -> texture_path, &face_size, map_cube_to_sphere);

	////////// Deciding which texture

	GLuint skybox_texture;

	if (map_cube_to_sphere) {
		const GLuint distorted_texture = make_spherically_distorted_skybox_texture(
			plain_texture, face_size, *spherical_distortion_config
		);

		deinit_texture(plain_texture);
		skybox_texture = distorted_texture;
	}
	else skybox_texture = plain_texture;

	//////////

	const GLuint shader = init_shader(ASSET_PATH("shaders/skybox.vert"), NULL, ASSET_PATH("shaders/skybox.frag"), NULL);
	use_shader(shader);
	use_texture_in_shader(skybox_texture, shader, "skybox_sampler", TexSkybox, TU_Skybox);

	return (Skybox) {
		init_drawable_with_vertices(define_skybox_vertex_spec, NULL, GL_STATIC_DRAW, GL_TRIANGLE_STRIP,
		(List) {.data = (void*) skybox_vertices_ts, .item_size = sizeof(skybox_vertices_ts[0]), .length = vertices_per_skybox},
		shader, skybox_texture, 0
		)
	};
}

void deinit_skybox(const Skybox* const skybox) {
	deinit_drawable(skybox -> drawable);
}

void draw_skybox(const Skybox* const skybox) {
	WITH_RENDER_STATE(glDepthFunc, GL_LEQUAL, GL_LESS, // Other depth testing mode for the skybox
		draw_drawable(skybox -> drawable, vertices_per_skybox, faces_per_cubemap, NULL, UseShaderPipeline | BindVertexSpec);
	);
}
