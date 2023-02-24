#include "rendering/ambient_occlusion.h"
#include "cglm/cglm.h" // For various cglm defs
#include "data/constants.h" // For TWO_PI, and `max_byte_value`
#include "utils/map_utils.h" // For `pos_out_of_overhead_map_bounds`, and `sample_map_point`
#include "utils/list.h" // For various `List`-related defs
#include "utils/macro_utils.h" // For `ARRAY_LENGTH`
#include "utils/texture.h" // For various texture creation utils
#include "utils/opengl_wrappers.h" // For various OpenGL wrappers

typedef uint16_t trace_count_t;
// TODO: define these in the `constants` struct, or make them parameters
static const trace_count_t num_trace_iters = 200;

//////////

static void generate_rand_dir(vec3 v) {
	static const GLfloat two_pi_over_rand_max = TWO_PI / RAND_MAX;

	const GLfloat // Within a unit sphere
		hori_angle = rand() * two_pi_over_rand_max,
		vert_angle = rand() * two_pi_over_rand_max;

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

//////////

static bool ray_collides_with_heightmap(
	const vec3 dir, const byte* const heightmap,
	const byte origin_x, const byte origin_y, const byte origin_z,
	const byte max_x, const byte max_y, const byte max_z,
	const bool is_dual_normal, const signed_byte flow[3]) {

	////////// https://www.shadertoy.com/view/3sKXDK

	signed_byte actual_flow[3];

	const signed_byte step_signs[3] = {
		(signed_byte) glm_signf(dir[0]),
		(signed_byte) glm_signf(dir[1]),
		(signed_byte) glm_signf(dir[2])
	};

	memcpy(actual_flow, is_dual_normal ? step_signs : flow, sizeof(signed_byte[3]));

	vec3 floating_origin = {origin_x, origin_y, origin_z};

	if (actual_flow[0] == -1) floating_origin[0] -= GLM_FLT_EPSILON;
	if (actual_flow[2] == -1) floating_origin[2] -= GLM_FLT_EPSILON;

	const int16_t start_pos[3] = {
		(int16_t) floorf(floating_origin[0]),
		(int16_t) floorf(floating_origin[1]),
		(int16_t) floorf(floating_origin[2])
	};

	//////////

	vec3 unit_step_size;
	glm_vec3_div(GLM_VEC3_ONE, (GLfloat*) dir, unit_step_size);
	glm_vec3_abs(unit_step_size, unit_step_size);

	vec3 ray_length_components = {
		(step_signs[0] == 1) ? (start_pos[0] + 1.0f - floating_origin[0]) : (floating_origin[0] - start_pos[0]),
		(step_signs[1] == 1) ? (start_pos[1] + 1.0f - floating_origin[1]) : (floating_origin[1] - start_pos[1]),
		(step_signs[2] == 1) ? (start_pos[2] + 1.0f - floating_origin[2]) : (floating_origin[2] - start_pos[2])
	};

	glm_vec3_mul(ray_length_components, unit_step_size, ray_length_components);

	int16_t curr_tile[3] = {start_pos[0], start_pos[1], start_pos[2]};

	//////////

	while (true) {
		// Will yield 1 if x > y, and 0 if x <= y (so this gives the index of the smallest among x and y)
		const byte x_and_y_min_index = ray_length_components[0] > ray_length_components[1];
		const byte index_of_shortest = (ray_length_components[x_and_y_min_index] > ray_length_components[2]) ? 2 : x_and_y_min_index;

		curr_tile[index_of_shortest] += step_signs[index_of_shortest];
		ray_length_components[index_of_shortest] += unit_step_size[index_of_shortest];

		//////////

		// TODO: Why are none of these ever negative?
		const int16_t cx = curr_tile[0], cy = curr_tile[1], cz = curr_tile[2]; // `c` = current

		// TODO: only check the component that changed
		if (cy > max_y || pos_out_of_overhead_map_bounds(cx, cz, max_x, max_z))
			return false;

		else if (cy < (int16_t) sample_map_point(heightmap, (byte) cx, (byte) cz, max_x))
			return true;
	}
}

//////////

static signed_byte sign_between_bytes(const byte a, const byte b) {
	if (a > b) return 1;
	else if (a < b) return -1;
	else return 0;
}

static signed_byte clamp_signed_byte_to_directional_range(const signed_byte x) {
	if (x > 1) return 1;
	else if (x < -1) return -1;
	else return x;
}

typedef struct {
	const vec3 normal;
	const signed_byte flow[3];
	const enum {OnMap, DualNormal, AboveMap, BelowMap} location_status;
} SurfaceNormalData;

SurfaceNormalData get_normal_data(const byte* const heightmap,
	const byte map_width, const byte x, const byte y, const byte z) {

	const byte left_x = (x == 0) ? 0 : (x - 1), top_z = (z == 0) ? 0 : (z - 1);

	const struct {const signed_byte tl, tr, bl, br;} diffs = {
		sign_between_bytes(sample_map_point(heightmap, left_x, top_z, map_width), y),
		sign_between_bytes(sample_map_point(heightmap, x, top_z, map_width), y),
		sign_between_bytes(sample_map_point(heightmap, left_x, z, map_width), y),
		sign_between_bytes(sample_map_point(heightmap, x, z, map_width), y)
	};

	//////////

	const signed_byte
		fx = clamp_signed_byte_to_directional_range((diffs.bl - diffs.br) + (diffs.tl - diffs.tr)),
		fy = !diffs.tl || !diffs.tr || !diffs.bl || !diffs.br,
		fz = clamp_signed_byte_to_directional_range((diffs.tl - diffs.bl) + (diffs.tr - diffs.br));

	if (fx == 0 && fy == 0 && fz == 0) {
		// Handling dual normals
		if (diffs.tl < diffs.bl || diffs.tr < diffs.tl)
			return (SurfaceNormalData) {GLM_VEC3_ZERO_INIT, {0, 0, 0}, DualNormal};

		////////// Above or below the map

		return (SurfaceNormalData) {
			GLM_VEC3_ZERO_INIT, {0, 0, 0},
			(diffs.tl == 1) ? BelowMap : AboveMap
		};
	}

	//////////

	vec3 normal = {fx, fy, fz};
	glm_vec3_normalize(normal);

	return (SurfaceNormalData) {{normal[0], normal[1], normal[2]}, {fx, fy, fz}, OnMap};
}

//////////

AmbientOcclusionMap init_ao_map(const byte* const heightmap, const byte map_size[2], const byte max_point_height) {
	/* TODO:
	- While debugging, use trilinear filtering to spot AO errors more easily
	- Compute the AO map through transform feedback; pass in 3D map points, and then pass back occlusion values

	- Give dual normals two sampling passes
	- Weight the collision result based on the distance that the ray traveled, or discard rays if they are too long

	- Implement the DDA version on the GPU (perhaps do separate raytraces per
		GPU thread for even more speed; but more memory consumed that way as well - perhaps define
		a trace iter split factor; some traces at some points are split up a bit into a few sub-passes?)

	- A little thought: rendering the scene from the point's perspective, with the direction of the normal,
		and then getting the fraction of the rendered thing covered by sky, would get the ambient occlusion term quite accurately

	- Later on, if it turns out that computing the AO map on the GPU is too slow, perhaps cache the AO map on disk
		(either computed at the first time that that level starts, or made by another program not tied to the game executable)

	- Maybe add a default value for above the map if high enough above all surrounding points?

	Order of resolution:
		- Some areas in a hemisphere or sphere go unsampled (normals are distributed well, but not rays that are cast, it seems)
		- Mip level bleed from black under-map values (oddly enough, giving a `constants.max_byte_value` value when under makes little difference)
		- Maybe the edge cases for DDA vs naive
		- Make tricubic less dark (it reads under the heightmap; to fix this, perhaps make the first layer
			of under-map values equal to the values above (but what about map sides then?))
		- Note: the tricubic darkness comes from its inherent overshoot; perhaps do a Lanzcos filter instead?
		- Either fix the overshoot by clamping in the fragment shader, changing the interpolation type, or making in-map values
			equal to the averages of the surrounding values outside
		- GPU
	*/

	//////////

	const byte max_x = map_size[0], max_z = map_size[1], max_y = max_point_height;
	byte* const ao_map = alloc(max_x * max_y * max_z, sizeof(byte));

	//////////

	srand((unsigned) time(NULL));

	vec3* const rand_dirs = alloc(num_trace_iters, sizeof(vec3));
	for (trace_count_t i = 0; i < num_trace_iters; i++) generate_rand_dir(rand_dirs[i]);

	//////////

	const GLfloat collision_term_scaler = (GLfloat) constants.max_byte_value / num_trace_iters;

	for (byte* dest = ao_map, y = 0; y < max_y; y++) { // For each posible height
		for (byte z = 0; z < max_z; z++) { // For each map row
			for (byte x = 0; x < max_x; x++, dest++) { // For each map point
				const SurfaceNormalData normal_data = get_normal_data(heightmap, max_x, x, y, z);

				if (normal_data.location_status == BelowMap) {
					*dest = 0;
					continue;
				}

				//////////

				const bool
					has_valid_normal = normal_data.location_status == OnMap,
					is_dual_normal = normal_data.location_status == DualNormal;

				trace_count_t num_collisions = 0;

				for (trace_count_t i = 0; i < num_trace_iters; i++) {
					vec3 aligned_rand_dir;
					glm_vec3_copy(rand_dirs[i], aligned_rand_dir);

					// TODO: for dual normals, do spherical sampling, or average the results of 2 sampling passes

					// If above the map, no negation based on normal needed, since sampling in a unit circle
					if (has_valid_normal && glm_vec3_dot((GLfloat*) normal_data.normal, aligned_rand_dir) < 0.0f)
						glm_vec3_negate(aligned_rand_dir);

					num_collisions += ray_collides_with_heightmap(aligned_rand_dir, heightmap, x, y, z,
						max_x, max_y, max_z, is_dual_normal, normal_data.flow);
				}

				*dest = constants.max_byte_value - (byte) (num_collisions * collision_term_scaler);
			}
		}
	}

	const GLuint texture = preinit_texture(TexVolumetric, TexNonRepeating, TexLinear, TexTrilinear, true);

	GLint prev_unpack_alignment;
	glGetIntegerv(GL_UNPACK_ALIGNMENT, &prev_unpack_alignment);
	glPixelStorei(GL_UNPACK_ALIGNMENT, sizeof(byte));

	init_texture_data(TexVolumetric, (GLsizei[]) {max_x, max_z, max_y}, GL_RED, OPENGL_AO_MAP_INTERNAL_PIXEL_FORMAT, OPENGL_COLOR_CHANNEL_TYPE, ao_map);
	glPixelStorei(GL_UNPACK_ALIGNMENT, prev_unpack_alignment);
	init_texture_mipmap(TexVolumetric);

	dealloc(ao_map);
	dealloc(rand_dirs);

	//////////

	return (AmbientOcclusionMap) {texture};
}

void deinit_ao_map(const AmbientOcclusionMap* const ao_map) {
	deinit_texture(ao_map -> texture);
}
