#include "rendering/ambient_occlusion.h"
#include "cglm/cglm.h" // For various cglm defs
#include "data/constants.h" // For `TWO_PI` and `DEBUG_AO_MAP_GENERATION`
#include "utils/uniform_buffer.h" // For various uniform buffer defs
#include "utils/alloc.h" // For `alloc`, and `dealloc`
#include "utils/shader.h" // For `init_shader`, and `deinit_shader`
#include "utils/texture.h" // For various texture creation utils
#include "utils/opengl_wrappers.h" // For various OpenGL wrappers

#ifdef DEBUG_AO_MAP_GENERATION
#include "utils/map_utils.h" // For `pos_out_of_overhead_map_bounds`, and `sample_map`
#endif

/* Maximum mipmaps, details:

Paper link: https://www.researchgate.net/publication/47862133
An implementation: https://github.com/kvark/vange-rs/pull/62
Commit where the implementation was removed: https://github.com/kvark/vange-rs/commit/f9d5a092e5d86b03b0dc90b88c471eb981cc5661

Preliminary notes:
	- Mip level n is the coarsest mip level (only one texel)
	- Mip level 0 is the finest mip level (no max function applied)
	- The ray will always be inside the bounds of the heightmap
	- This algorithm is fast because of space skipping: if the ray is over a mip texel, it's over all heightmap texels in that area

Construction:
	1. Round map size to a power of 2, and pad bottom and right edges
	2. Build a maximum mipmap, ignoring the edges

Algorithm:
	define a ray with an origin, a direction, and a current position

	mip_level = num_mips - 1
	ray_above_heightmap = true

	while ray_above_heightmap:
		sampled_height = sample_height(ray.pos, ray.dir, mip_level)

		##########

		if mip_level == 0:
			if ray.end.y <= sampled_height:
				update_ray_pos_for_final_intersecting_box()
				ray_above_heightmap = false
		else:
			update_ray_pos_for_mip_level()

		# Not in the paper, but I added this little snippet
		if ray outside heightmap bounds: ray_above_heightmap = false

		##########

		if ray.end.y > sampled_height || mip_level == 0:
			TODO: figure out
		else:
			mip_level -= 1
*/

/* TODO:
Optimization:
- Spatial hashing for using less space
- Tracing via maximum mipmaps, or a quadtree/octree/r-tree tracing method
- Result caching: first sort the rays, and if the previous ray collided, test if the current ray collides with the same point (or around that area); if so, skip this tracing step. Only works for densely distributed rays.
- (DONE) Allow for a workload split factor, to trade more speed for more space used
- Allow for a batch fraction size factor, to trade less speed for less space used
- If reading back directly after the draw call is still too slow, then perhaps do a readback at the end of all level initialization
- If creating the AO map is too slow, then cache it on disk (a cache directory per level)
- Maybe add a default value for above the map if high enough above all surrounding points?

Fixes:
- The aliasing involved in the filtering (perhaps it happens because the world-space position fluctuates so much for larger viewing distances?)
- Unsampled areas within the hemisphere or sphere (normals are distributed well, but not the rays that are cast, it seems)
- Give dual normals two sampling passes, and average the results
- Figure out a better sampling method for over the map (sampling downwards doesn't make sense, since you should be going towards a light source)
- Perhaps don’t make the bottom of the map black (oddly enough, giving a maximum-ao value when under makes little difference), maybe average neighboring overhead values iteratively?)
- Perhaps use cosine weighting, instead of just a straight average (or some type of importance sampling, for less noise with fewer samples). See here: https://alexanderameye.github.io/notes/sampling-the-hemisphere/

Possible:
- Output to some target that has byte-size components
- Only check the component that changed in the inner loop, if possible
- (DONE, NO DIFFERENCE) See if disabling face culling during transform feedback changes performance at all
- See if direction sorting could help at all
- Would ray attenuation make anything look better?
- Stratified sampling might look better with less samples
- Rendering from each point with a given direction could maybe serve as a raytrace, in a way (may be hard to parallelize)

Miscellaneous:
- (DONE) Keep a CPU implementation of the algorithm (removed by default by a conditional macro), in order to allow verification of the GPU implementation
- Define the number of trace iters and the max number of ray steps (and the new constants above) as part of the level JSON
- A Lanzcos filter instead, to fix the tricubic overshoot?
*/

////////// Some typedefs

/*
T = the total thread count
N = the number of trace iters per point
P = the number of points on the 3D grid
W = the workload split factor

- Normally, N loop iterations are run per thread, with each thread handling one point.
- But with this, N/W loop iterations are done per thread, with T*W total threads being spawned.
- This means that W threads correspond to one point.

Overall, having this split factor should save GPU time, by parallelizing
the workload more. It does use W times more memory though.
*/

typedef uint8_t workload_split_factor_t;
typedef uint8_t ao_value_t;
typedef uint16_t trace_count_t;
typedef uint16_t ray_step_count_t;

typedef struct {
	/* TODO: why does increasing the workload split factor only make things slower?
	That doesn't make any sense. Also, see that this is nondestructive, in regards to
	divisions or moduli.*/
	const workload_split_factor_t workload_split_factor;
	const trace_count_t num_trace_iters;
	const ray_step_count_t max_num_ray_steps;
} AmbientOcclusionComputeParams;

////////// Some constants

// TODO: make this a param to `init_ao_map`
static const AmbientOcclusionComputeParams compute_params = {
	.workload_split_factor = 1u,
	.num_trace_iters = 512u,
	.max_num_ray_steps = 20u
};

static const GLfloat float_epsilon = GLM_FLT_EPSILON;

////////// Some general utils

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
		if (*component == 0.0f) *component = float_epsilon;
	}

	// Normalizing it, just to make 100% sure that it's a unit vector
	glm_vec3_normalize(v);
}

static ao_value_t get_ao_term_from_collision_count(const trace_count_t num_collisions) {
	static const ao_value_t max_ao_value = (ao_value_t) ~0u;
	const GLfloat collision_term_scaler = (GLfloat) max_ao_value / compute_params.num_trace_iters;

	return max_ao_value - (ao_value_t) (num_collisions * collision_term_scaler);
}

////////// The CPU implementation of the GPU AO algorithm

#ifdef DEBUG_AO_MAP_GENERATION

static bool ray_collides_with_heightmap(const vec3 dir,
	const Heightmap heightmap, const map_pos_component_t max_y,
	const map_pos_component_t origin_x, const map_pos_component_t origin_y,
	const map_pos_component_t origin_z, const bool is_dual_normal, const sbvec3 flow) {

	const sbvec3 step_signs = {
		(signed_byte) glm_signf(dir[0]),
		(signed_byte) glm_signf(dir[1]),
		(signed_byte) glm_signf(dir[2])
	};

	const sbvec3 actual_flow = is_dual_normal ? step_signs : flow;

	const vec3 floating_origin = {
		(actual_flow.x == -1) * -float_epsilon + origin_x,
		origin_y,
		(actual_flow.z == -1) * -float_epsilon + origin_z
	};

	const vec3 start_pos = {floorf(floating_origin[0]), floorf(floating_origin[1]), floorf(floating_origin[2])};

	//////////

	vec3 unit_step_size;
	glm_vec3_div(GLM_VEC3_ONE, (GLfloat*) dir, unit_step_size);
	glm_vec3_abs(unit_step_size, unit_step_size);

	vec3 ray_length_components;
	glm_vec3_sub((GLfloat*) floating_origin, (GLfloat*) start_pos, ray_length_components);

	for (byte i = 0; i < 3; i++) {
		const GLfloat component = ray_length_components[i];
		ray_length_components[i] = (step_signs[i] == 1) ? 1.0f - component : component;
	}

	glm_vec3_mul(ray_length_components, unit_step_size, ray_length_components);

	//////////

	signed_map_pos_component_t curr_tile[3] = {
		(signed_map_pos_component_t) start_pos[0],
		(signed_map_pos_component_t) start_pos[1],
		(signed_map_pos_component_t) start_pos[2]
	};

	for (ray_step_count_t i = 0; i < compute_params.max_num_ray_steps; i++) {
		// Will yield 1 if x > y, and 0 if x <= y (so this gives the index of the smallest among x and y)
		const byte x_and_y_min_index = (ray_length_components[0] > ray_length_components[1]);
		const byte index_of_shortest = (ray_length_components[x_and_y_min_index] > ray_length_components[2]) ? 2 : x_and_y_min_index;

		curr_tile[index_of_shortest] += step_signs[index_of_shortest];
		ray_length_components[index_of_shortest] += unit_step_size[index_of_shortest];

		//////////

		const signed_map_pos_component_t cx = curr_tile[0], cy = curr_tile[1], cz = curr_tile[2]; // `c` = current

		if (cy < 0) return true;
		else if (cy > max_y || pos_out_of_overhead_map_bounds((vec2) {cx, cz}, heightmap.size)) return false;

		else if (cy < (signed_map_pos_component_t) sample_map(heightmap,
			(map_pos_xz_t) {(map_pos_component_t) cx, (map_pos_component_t) cz})) return true;
	}

	return false;
}

//////////

static signed_byte sign_between_map_values(const map_pos_component_t a, const map_pos_component_t b) {
	if (a > b) return 1;
	else if (a < b) return -1;
	else return 0;
}

static signed_byte clamp_signed_byte_to_directional_range(const signed_byte x) {
	if (x > 1) return 1;
	else if (x < -1) return -1;
	else return x;
}

typedef enum {
	DualNormal, OnMap, AboveMap, BelowMap
} NormalLocationStatus;

typedef struct {
	const vec3 normal;
	const sbvec3 flow;
	const NormalLocationStatus location_status;
} SurfaceNormalData;

//////////

SurfaceNormalData get_normal_data(const Heightmap heightmap,
	const map_pos_component_t x, const map_pos_component_t y, const map_pos_component_t z) {

	const map_pos_component_t left_x = (x == 0) ? 0 : (x - 1), top_z = (z == 0) ? 0 : (z - 1);

	const struct {const signed_byte tl, tr, bl, br;} diffs = {
		sign_between_map_values(sample_map(heightmap, (map_pos_xz_t) {left_x, top_z}), y),
		sign_between_map_values(sample_map(heightmap, (map_pos_xz_t) {x, top_z}), y),
		sign_between_map_values(sample_map(heightmap, (map_pos_xz_t) {left_x, z}), y),
		sign_between_map_values(sample_map(heightmap, (map_pos_xz_t) {x, z}), y)
	};

	//////////

	const signed_byte
		fx = clamp_signed_byte_to_directional_range((diffs.bl - diffs.br) + (diffs.tl - diffs.tr)),
		fy = !diffs.tl || !diffs.tr || !diffs.bl || !diffs.br,
		fz = clamp_signed_byte_to_directional_range((diffs.tl - diffs.bl) + (diffs.tr - diffs.br));

	if (fx == 0 && fy == 0 && fz == 0) {
		NormalLocationStatus location_status;

		// Handling dual normals
		if (diffs.tl < diffs.bl || diffs.tr < diffs.tl) location_status = DualNormal;
		else location_status = (diffs.tl == 1) ? BelowMap : AboveMap;

		return (SurfaceNormalData) {GLM_VEC3_ZERO_INIT, {0, 0, 0}, location_status};
	}

	//////////

	vec3 normal = {fx, fy, fz};
	glm_vec3_normalize(normal);

	return (SurfaceNormalData) {{normal[0], normal[1], normal[2]}, {fx, fy, fz}, OnMap};
}

#endif

//////////

static void transform_feedback_hook(const GLuint shader) {
	glTransformFeedbackVaryings(shader, 1, (const GLchar*[]) {"num_collisions"}, GL_INTERLEAVED_ATTRIBS);
}

static GLuint init_ao_map_texture(const Heightmap heightmap, const map_pos_component_t max_y,
	const vec3* const rand_dirs, ao_value_t** const transform_feedback_data_ref) {

	////////// Defining some constants

	typedef GLint transform_feedback_output_t;

	const TextureType heightmap_texture_type = TexRect;

	const GLenum
		heightmap_input_format = GL_RED_INTEGER,
		transform_feedback_buffer_target = GL_TRANSFORM_FEEDBACK_BUFFER,

		// TODO: ensure that these usages are optimal
		transform_feedback_buffer_usage = GL_STATIC_DRAW,
		rand_dirs_ubo_usage = GL_STATIC_DRAW;

	const GLuint transform_feedback_buffer_binding_point = 0;
	const GLint heightmap_internal_pixel_format = GL_R8UI;

	////////// Setting the unpack alignment for the heightmap and output textures

	GLint prev_unpack_alignment;
	glGetIntegerv(GL_UNPACK_ALIGNMENT, &prev_unpack_alignment);
	glPixelStorei(GL_UNPACK_ALIGNMENT, sizeof(ao_value_t));

	////////// Some variable initialization

	const buffer_size_t num_points_on_grid = heightmap.size.x * max_y * heightmap.size.z;
	const buffer_size_t num_threads = num_points_on_grid * compute_params.workload_split_factor;
	const buffer_size_t output_buffer_size = num_threads * sizeof(transform_feedback_output_t);

	const GLuint transform_feedback_gpu_buffer = init_gpu_buffer();
	use_gpu_buffer(transform_feedback_buffer_target, transform_feedback_gpu_buffer);
	init_gpu_buffer_data(transform_feedback_buffer_target, 1, output_buffer_size, NULL, transform_feedback_buffer_usage);
	glBindBufferBase(transform_feedback_buffer_target, transform_feedback_buffer_binding_point, transform_feedback_gpu_buffer);

	////////// Making a heightmap texture

	const GLuint heightmap_texture = preinit_texture(heightmap_texture_type, TexNonRepeating, TexNearest, TexNearest, false);

	init_texture_data(heightmap_texture_type, (GLsizei[]) {heightmap.size.x, heightmap.size.z},
		heightmap_input_format, heightmap_internal_pixel_format, MAP_POS_COMPONENT_TYPENAME, heightmap.data);

	////////// Making a shader, and using it

	const GLuint shader = init_shader("shaders/precompute_ambient_occlusion.vert", NULL, NULL, transform_feedback_hook);
	use_shader(shader);

	////////// Writing the plain uniforms and the heightmap to the shader

	const trace_count_t num_traces_per_thread = compute_params.num_trace_iters / compute_params.workload_split_factor;

	if (num_traces_per_thread * compute_params.workload_split_factor != compute_params.num_trace_iters)
		FAIL(CreateTexture, "Cannot compute an ambient occlusion texture because the %u "
			"trace iterations per point cannot be split evenly into %u thread groups",
			compute_params.num_trace_iters, compute_params.workload_split_factor);

	INIT_UNIFORM_VALUE(workload_split_factor, shader, 1ui, compute_params.workload_split_factor);
	INIT_UNIFORM_VALUE(num_traces_per_thread, shader, 1ui, num_traces_per_thread);
	INIT_UNIFORM_VALUE(max_num_ray_steps, shader, 1ui, compute_params.max_num_ray_steps);
	INIT_UNIFORM_VALUE(max_point_height, shader, 1ui, max_y);
	INIT_UNIFORM_VALUE(float_epsilon, shader, 1f, float_epsilon);

	use_texture_in_shader(heightmap_texture, shader, "heightmap_sampler", heightmap_texture_type, TU_Temporary);

	////////// Making a uniform buffer, and writing the random dirs to it

	const GLchar* const rand_dirs_uniform_name = "rand_dirs";

	UniformBuffer rand_dirs_ubo = init_uniform_buffer(rand_dirs_ubo_usage,
		"RandDirs", shader, (const GLchar*[]) {rand_dirs_uniform_name}, 1);

	bind_uniform_buffer_to_shader(&rand_dirs_ubo, shader);
	enable_uniform_buffer_writing_batch(&rand_dirs_ubo, true);

	write_array_of_primitives_to_uniform_buffer(&rand_dirs_ubo,
		rand_dirs_uniform_name, (List) {
			.data = (void*) rand_dirs,
			.item_size = sizeof(vec3),
			.length = compute_params.num_trace_iters
		}
	);

	disable_uniform_buffer_writing_batch(&rand_dirs_ubo);

	////////// Drawing

	WITH_BINARY_RENDER_STATE(GL_RASTERIZER_DISCARD,
		glBeginTransformFeedback(GL_POINTS);
		draw_primitives(GL_POINTS, (GLsizei) num_threads);
		glEndTransformFeedback();
	);

	////////// Reading back from the feedback GPU buffer

	transform_feedback_output_t* const raw_transform_feedback_data = init_gpu_buffer_memory_mapping(
		transform_feedback_gpu_buffer, transform_feedback_buffer_target, output_buffer_size, false);

	ao_value_t* const transform_feedback_data = alloc(num_points_on_grid, sizeof(ao_value_t));

	for (buffer_size_t i = 0; i < num_points_on_grid; i++) {
		const buffer_size_t raw_transform_feedback_data_index = i * compute_params.workload_split_factor;
		const transform_feedback_output_t first_collision_sum = raw_transform_feedback_data[raw_transform_feedback_data_index];

		/* If the first one is -1, that means that the current span
		of values for this point's workload is under the map */
		if (first_collision_sum == -1) transform_feedback_data[i] = 0;
		else {
			// This will not overflow, since the total collision sum will be under the max trace count
			trace_count_t total_collision_sum = (trace_count_t) first_collision_sum;

			for (workload_split_factor_t j = 1; j < compute_params.workload_split_factor; j++)
				total_collision_sum += raw_transform_feedback_data[raw_transform_feedback_data_index + j];

			transform_feedback_data[i] = get_ao_term_from_collision_count(total_collision_sum);
		}
	}

	deinit_gpu_buffer_memory_mapping(transform_feedback_buffer_target);
	*transform_feedback_data_ref = transform_feedback_data;

	////////// Making an output texture

	const GLuint output_texture = preinit_texture(TexVolumetric, TexNonRepeating, TexLinear, TexTrilinear, true);

	init_texture_data(TexVolumetric, (GLsizei[]) {heightmap.size.x, heightmap.size.z, max_y}, GL_RED,
		OPENGL_AO_MAP_INTERNAL_PIXEL_FORMAT, OPENGL_COLOR_CHANNEL_TYPE, transform_feedback_data);

	init_texture_mipmap(TexVolumetric);

	// Resetting the unpack alignment
	glPixelStorei(GL_UNPACK_ALIGNMENT, prev_unpack_alignment);

	////////// Deinit

	deinit_texture(heightmap_texture);
	deinit_uniform_buffer(&rand_dirs_ubo);
	deinit_gpu_buffer(transform_feedback_gpu_buffer);
	deinit_shader(shader);

	return output_texture;
}

AmbientOcclusionMap init_ao_map(const Heightmap heightmap, const map_pos_component_t max_y) {
	////////// Seeding the random generator, and generating random dirs

	/* TODO: fix the possible intersection problems for height
	values over 255. For example, with seed 1680397591u. */

	const unsigned seed = (unsigned) time(NULL);
	srand(seed);

	vec3* const rand_dirs = alloc(compute_params.num_trace_iters, sizeof(vec3));
	for (trace_count_t i = 0; i < compute_params.num_trace_iters; i++) generate_rand_dir(rand_dirs[i]);

	////////// Making an AO map on the GPU

	ao_value_t* transform_feedback_data;
	const GLuint ao_map_texture = init_ao_map_texture(heightmap, max_y, rand_dirs, &transform_feedback_data);

	////////// Verifying that the AO map computed on the GPU is correct

	#ifdef DEBUG_AO_MAP_GENERATION

	printf("The seed provided to `srand` = %uu\nStarting the CPU algorithm\n", seed);

	uint64_t num_correct_with_transform_feedback = 0;
	const ao_value_t* curr_transform_feedback_datum = transform_feedback_data;

	for (map_pos_component_t y = 0; y < max_y; y++) { // For each posible height
		printf("%g%%\n", (GLdouble) y / max_y * 100.0);
		for (map_pos_component_t z = 0; z < heightmap.size.z; z++) { // For each map row
			for (map_pos_component_t x = 0; x < heightmap.size.x; x++, curr_transform_feedback_datum++) { // For each map point
				const SurfaceNormalData normal_data = get_normal_data(heightmap, x, y, z);

				const ao_value_t transform_feedback_result = *curr_transform_feedback_datum;

				if (normal_data.location_status == BelowMap) {
					if (transform_feedback_result == 0) num_correct_with_transform_feedback++;
					continue;
				}

				//////////

				const bool
					has_valid_normal = normal_data.location_status == OnMap,
					is_dual_normal = normal_data.location_status == DualNormal;

				trace_count_t num_collisions = 0;

				for (trace_count_t i = 0; i < compute_params.num_trace_iters; i++) {
					vec3 rand_dir;
					glm_vec3_copy(rand_dirs[i], rand_dir);

					// If above the map, no negation based on normal needed, since sampling in a unit circle
					if (has_valid_normal && glm_vec3_dot((GLfloat*) normal_data.normal, rand_dir) < 0.0f)
						glm_vec3_negate(rand_dir);

					num_collisions += ray_collides_with_heightmap(rand_dir,
						heightmap, max_y, x, y, z, is_dual_normal, normal_data.flow);
				}

				const ao_value_t cpu_result = get_ao_term_from_collision_count(num_collisions);
				if (cpu_result == transform_feedback_result) num_correct_with_transform_feedback++;

				else printf("Incorrect. pos = {%u, %u, %u}. Results: %u vs %u; num colls = %u\n",
					x, y, z, cpu_result, transform_feedback_result, num_collisions);
			}
		}
	}

	printf("Finished the CPU algorithm\n%g%% of the CPU results match the GPU results\n",
		(GLdouble) num_correct_with_transform_feedback / (heightmap.size.x * max_y * heightmap.size.z) * 100.0);

	#endif

	////////// Deinit

	dealloc(rand_dirs);
	dealloc(transform_feedback_data);

	return (AmbientOcclusionMap) {ao_map_texture};
}

void deinit_ao_map(const AmbientOcclusionMap* const ao_map) {
	deinit_texture(ao_map -> texture);
}
