#include "rendering/ambient_occlusion.h"
#include "utils/uniform_buffer.h"
#include "data/constants.h"
#include "utils/shader.h"
#include "utils/texture.h"

/* TODO:
- Define these in the `constants` struct
- Possibly make the number of trace iters correspond directly to `constants.max_byte_value` */
static const byte num_trace_iters = 255, rand_seed = 85;

//////////

static GLfloat generate_rand_number_within_range(const GLfloat min, const GLfloat max) {
	static const GLfloat one_over_rand_max = 1.0f / RAND_MAX;

	// `https://stackoverflow.com/questions/13408990/how-to-generate-random-float-number-in-c`, under baz's answer
	const GLfloat scale = rand() * one_over_rand_max;
	return scale * (max - min) + min;
}

static void generate_random_dir(vec3 v) {
	const GLfloat h_min = 0.0f, h_max = TWO_PI, v_min = 0.0f, v_max = TWO_PI; // Sphere
	// const GLfloat h_min = 0.0f, h_max = TWO_PI, v_min = 0.0f, v_max = PI; // Flat hemisphere

	const GLfloat
		hori_angle = generate_rand_number_within_range(h_min, h_max),
		vert_angle = generate_rand_number_within_range(v_min, v_max);

	const GLfloat cos_vert = cosf(vert_angle);

	v[0] = cos_vert * sinf(hori_angle);
	v[1] = sinf(vert_angle);
	v[2] = cos_vert * cosf(hori_angle);

	// This is to avoid any divisions by zero
	for (byte i = 0; i < 3; i++) {
		GLfloat* const component = v + i;
		if (*component == 0.0f) *component = GLM_FLT_EPSILON;
	}
}

static bool ray_collides_with_heightmap(
	const vec3 inv_dir, const byte* const heightmap,
	const byte origin_x, const byte origin_y, const byte origin_z,
	const byte max_x, const byte max_y, const byte max_z) {

	////////// https://github.com/cgyurgyik/fast-voxel-traversal-algorithm/

	vec3 unit_step_size;
	glm_vec3_abs((GLfloat*) inv_dir, unit_step_size);

	vec3 ray_length_components = {
		(inv_dir[0] > 0.0f) ? inv_dir[0] : 0.0f,
		(inv_dir[1] > 0.0f) ? inv_dir[1] : 0.0f,
		(inv_dir[2] > 0.0f) ? inv_dir[2] : 0.0f
	};

	const signed_byte tile_steps[3] = {
		(signed_byte) glm_signf(inv_dir[0]),
		(signed_byte) glm_signf(inv_dir[1]),
		(signed_byte) glm_signf(inv_dir[2])
	};

	//////////

	int16_t curr_tile[3] = {origin_x, origin_y, origin_z};

	byte component_change_bits = 0;
	const byte mask_for_all_changed_components = 7u; // = 0b111

	while (true) {
		// Will yield 1 if x >= y, and 0 if x < y (so this gives the index of the smallest among x and y)
		const byte x_and_y_min_index = ray_length_components[0] >= ray_length_components[1];
		const byte index_of_shortest = (ray_length_components[x_and_y_min_index] < ray_length_components[2]) ? x_and_y_min_index : 2;

		curr_tile[index_of_shortest] += tile_steps[index_of_shortest];
		ray_length_components[index_of_shortest] += unit_step_size[index_of_shortest];

		component_change_bits |= (1 << index_of_shortest); // Setting the right bit indicating a component change

		//////////

		if (curr_tile[1] > max_y || // TODO: only check the component that changed
			curr_tile[0] == -1 || curr_tile[0] == max_x ||
			curr_tile[2] == -1 || curr_tile[2] == max_z) return false;

		else if (curr_tile[1] < (int16_t) sample_map_point(heightmap, (byte) curr_tile[0], (byte) curr_tile[2], max_x)) {
			if (component_change_bits == mask_for_all_changed_components) return true;
		}
	}
}

// TODO: make this functional
// https://stackoverflow.com/questions/33736199/calculating-normals-for-a-height-map
static void get_normal_at(
	const byte* const heightmap,
	const byte map_width, const byte map_height,
	const byte x, const byte y, const byte z, vec3 normal) {

	/* TODO: eventually, when not doing things that touch the heightmap directly,
	compute the heights around the center as relative to the center */

	const byte
		left_x = (x == 0) ? 0 : (x - 1), right_x = (x == map_width - 1) ? x : (x + 1),
		top_z = (z == 0) ? 0 : (z - 1), bottom_z = (z == map_height - 1) ? z : (z + 1);

	const byte
		h_center = sample_map_point(heightmap, x, z, map_width),
		h_above = sample_map_point(heightmap, x, top_z, map_width),
		h_below = sample_map_point(heightmap, x, bottom_z, map_width),
		h_left = sample_map_point(heightmap, left_x, z, map_width),
		h_right = sample_map_point(heightmap, right_x, z, map_width);

	#define s_diff(a, b) glm_signf((GLfloat) (a) - (GLfloat) (b))

	const GLfloat
		sign_horizontal = s_diff(s_diff(h_left, h_center), s_diff(h_right, h_center)),
		sign_vertical = s_diff(s_diff(h_above, h_center), s_diff(h_below, h_center));

	#undef s_diff

	normal[0] = sign_horizontal;
	normal[1] = y == h_center;
	normal[2] = sign_vertical;

	glm_vec3_normalize(normal);
}

// TODO: remove
static void normal_inference_unit_test(void) {
	enum {map_height = 5, map_width = 5};

	const byte F = 0;

	const byte heightmap[map_height][map_width] = {
		{0, 0, 0, 0, 0},
		{2, 2, 2, 0, 0},
		{2, F, 2, 0, 0},
		{2, 2, 2, 0, 0},
		{0, 0, 0, 0, 0}
	};

	const byte chosen_pos[3] = {1, 0, 2};

	vec3 normal;

	get_normal_at((const byte*) heightmap, map_width, map_height,
		chosen_pos[0], chosen_pos[1], chosen_pos[2], normal);

	DEBUG_VEC3(normal);
}

AmbientOcclusionMap init_ao_map(const byte* const heightmap, const byte map_size[2], const byte max_point_height) {
	/* TODO:
	- Eliminate any bilinear filtering artifacts through correct raycasting + bicubic filtering (ask on Stackoverflow)
	- Some map boundary edges are fully light
	- Compute the AO map through transform feedback; pass in 3D map points, and then pass back occlusion values
	- Making an AO map for the terrain fails
	- A weird stitch-type bug when looking at a surface at a sharp angle
	- Extend max x and z to 1+ on each
	- For the GPU version, height level 0 is too bright
	- To find the difference between the CPU and GPU raytracers, perhaps remove everything from each respective function,
		and then add little bits and pieces one at a time until something doesn't line up
	- Trace in the opposite direction - from the sky into the world?
	- Note: there will be no occlusion leaking once this hemisphere stuff is figured out
	- It doesn't seem dark enough inside blocks

	Immediate plan:
		- For a given vertex, find a hemisphere in which rays will be cast outwards from
		- First, generate rays around a hemisphere in an outer loop
		- Then, for each vertex, find a face normal which defines the angle range/vector area.
		- Only generate rand vals within a hemisphere, and then rotate each one to fit within the surface normal's hemisphere
		- For each hemisphere orientation, generate a rotation matrix, and then select one based on the vertex normal

	- Implement the DDA version on the GPU
	*/

	//////////

	const byte max_x = map_size[0], max_z = map_size[1], max_y = max_point_height + 1;
	// printf("size = {%hhu, %hhu, %hhu}\n", max_x, max_y, max_z);

	byte* const ao_map = alloc(max_x * max_y * max_z, sizeof(byte));
	vec3* const inv_rand_dirs = alloc(num_trace_iters, sizeof(vec3));

	srand(rand_seed);

	for (byte i = 0; i < num_trace_iters; i++) {
		GLfloat* const v = inv_rand_dirs[i];
		generate_random_dir(v);
		glm_vec3_div(GLM_VEC3_ONE, v, v);
	}

	const GLfloat collision_term_scaler = (GLfloat) constants.max_byte_value / num_trace_iters;

	(void) normal_inference_unit_test;
	// normal_inference_unit_test(); // TODO: make this correct

	for (byte* dest = ao_map, y = 0; y < max_y; y++) { // For each posible height
		for (byte z = 0; z < max_z; z++) { // For each map row
			for (byte x = 0; x < max_x; x++, dest++) { // For each map point

				vec3 normal;
				get_normal_at(heightmap, max_x, max_z, x, y, z, normal);

				byte num_collisions = 0;

				for (byte i = 0; i < num_trace_iters; i++) {
					GLfloat* const inv_rand_dir = inv_rand_dirs[i];
					if (glm_vec3_dot(normal, inv_rand_dir) < 0.0f) glm_vec3_negate(inv_rand_dir);

					num_collisions += ray_collides_with_heightmap(inv_rand_dir, heightmap, x, y, z, max_x, max_y, max_z);
				}

				*dest = constants.max_byte_value - (byte) (num_collisions * collision_term_scaler);
			}
		}
	}

	const GLuint texture = preinit_texture(TexVolumetric, TexNonRepeating, TexLinear, TexTrilinear, false);

	GLint standard_unpack_alignment;
	glGetIntegerv(GL_UNPACK_ALIGNMENT, &standard_unpack_alignment);
	glPixelStorei(GL_UNPACK_ALIGNMENT, sizeof(byte)); // TODO: specify the format in texture.h, and use sRGB

	init_texture_data(TexVolumetric, (GLsizei[]) {map_size[0], map_size[1], max_y}, GL_RED, GL_RED, OPENGL_COLOR_CHANNEL_TYPE, ao_map);

	glPixelStorei(GL_UNPACK_ALIGNMENT, standard_unpack_alignment);
	init_texture_mipmap(TexVolumetric);

	dealloc(ao_map);
	dealloc(inv_rand_dirs);

	return (AmbientOcclusionMap) {texture};
}

void deinit_ao_map(const AmbientOcclusionMap* const ao_map) {
	deinit_texture(ao_map -> texture);
}
