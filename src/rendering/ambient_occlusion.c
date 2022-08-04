#include "rendering/ambient_occlusion.h"
#include "data/constants.h"
#include "utils/texture.h"

// TODO: define these in the `constants` struct
static const byte num_trace_iters = 255, rand_seed = 5;

//////////

// TODO: check that no component ever equals 0
static void generate_random_dir(vec3 v) {
	// Multiplying this by a number in the range of [0, RAND_MAX] gives you a number in the range [0, TWO_PI]
	static const GLfloat convert_to_cos_sin_period = TWO_PI / RAND_MAX;

	const GLfloat
		vert_angle = rand() * convert_to_cos_sin_period,
		hori_angle = rand() * convert_to_cos_sin_period;

	const GLfloat cos_vert = cosf(vert_angle);

	v[0] = cos_vert * sinf(hori_angle);
	v[1] = sinf(vert_angle);
	v[2] = cos_vert * cosf(hori_angle);
}

static bool ray_collides_with_heightmap(
	const vec3 inv_dir, const byte* const heightmap,
	const byte origin_x, const byte origin_y, const byte origin_z,
	const byte max_x, const byte max_y, const byte max_z) {

	////////// https://github.com/cgyurgyik/fast-voxel-traversal-algorithm/

	vec3 unit_step_size;
	glm_vec3_abs((GLfloat*) inv_dir, unit_step_size);

	const bool x_dir_positive = inv_dir[0] > 0.0f, y_dir_positive = inv_dir[1] > 0.0f, z_dir_positive = inv_dir[2] > 0.0f;
	vec3 ray_length_components = {inv_dir[0] * x_dir_positive, inv_dir[1] * y_dir_positive, inv_dir[2] * z_dir_positive};
	const signed_byte tile_steps[3] = {x_dir_positive ? 1 : -1, y_dir_positive ? 1 : -1, z_dir_positive ? 1 : -1};

	//////////

	int16_t curr_tile[3] = {origin_x, origin_y, origin_z};

	while (true) {
		byte index_of_shortest;

		if (ray_length_components[0] < ray_length_components[1])
			index_of_shortest = (ray_length_components[0] < ray_length_components[2]) ? 0 : 2;
		else
			index_of_shortest = (ray_length_components[1] < ray_length_components[2]) ? 1 : 2;

		curr_tile[index_of_shortest] += tile_steps[index_of_shortest];
		ray_length_components[index_of_shortest] += unit_step_size[index_of_shortest];

		if (curr_tile[1] > max_y || // TODO: only check the component that changed
			curr_tile[0] == -1 || curr_tile[0] > max_x ||
			curr_tile[2] == -1 || curr_tile[2] > max_z) return false;

		const byte sample_height = sample_map_point(heightmap, (byte) curr_tile[0], (byte) curr_tile[2], max_x);
		if (curr_tile[1] < sample_height || curr_tile[1] == -1) return true;
	}
}

AmbientOcclusionMap init_ao_map(const byte* const heightmap, const byte map_size[2], const byte max_point_height) {
	/* TODO:
	- Eliminate any bilinear filtering artifacts
	- Some map boundary edges are fully light
	- Figure out if I need the hemisphere multiplication
	- Fix occlusion leaking
	- Compute the AO map through transform feedback; pass in 3D map points, and then pass back occlusion values
	- Making an AO map for the terrain fails
	- Put the AO map and the inv rand dirs on the heap
	*/

	//////////

	const byte
		max_x = map_size[0], max_z = map_size[1],
		max_y = max_point_height + 1;

	byte ao_map[max_y][max_z][max_x];

	srand(rand_seed);
	vec3 inv_rand_dirs[num_trace_iters];

	for (byte i = 0; i < num_trace_iters; i++) {
		GLfloat* const inv_rand_dir = inv_rand_dirs[i];
		generate_random_dir(inv_rand_dir);
		glm_vec3_div(GLM_VEC3_ONE, inv_rand_dir, inv_rand_dir);
	}

	const GLfloat collision_term_scaler = (GLfloat) constants.max_byte_value / num_trace_iters;

	//////////

	for (byte y = 0; y < max_y; y++) { // For each posible height
		for (byte z = 0; z < max_z; z++) { // For each map row
			for (byte x = 0; x < max_x; x++) { // For each map point
				byte* const dest = &ao_map[y][z][x];

				// Skipping points that are under the heightmap (TODO: add this back)
				/*
				if (y < sample_map_point(heightmap, x, z, max_x)) {
					*dest = 0;
					continue;
				}
				*/

				byte num_collisions = 0;

				for (byte i = 0; i < num_trace_iters; i++)
					num_collisions += ray_collides_with_heightmap(inv_rand_dirs[i], heightmap, x, y, z, max_x, max_y, max_z);

				*dest = constants.max_byte_value - (byte) (num_collisions * collision_term_scaler);
			}
		}
	}

	const GLuint texture = preinit_texture(TexVolumetric, TexNonRepeating, TexLinear, TexTrilinear, false);

	GLint prev_unpack_alignment;
	glGetIntegerv(GL_UNPACK_ALIGNMENT, &prev_unpack_alignment);
	glPixelStorei(GL_UNPACK_ALIGNMENT, sizeof(byte)); // TODO: specify the format in texture.h, and use sRGB

	init_texture_data(TexVolumetric, (GLsizei[]) {map_size[0], map_size[1], max_y}, GL_RED, GL_RED, OPENGL_COLOR_CHANNEL_TYPE, ao_map);

	glPixelStorei(GL_UNPACK_ALIGNMENT, prev_unpack_alignment);
	glGenerateMipmap(TexVolumetric);

	return texture;
}

void deinit_ao_map(const AmbientOcclusionMap ao_map) {
	glDeleteTextures(1, &ao_map);
}
