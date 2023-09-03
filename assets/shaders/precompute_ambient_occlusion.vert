#version 400 core

////////// Constants

// TODO: make this an input in some type of file like `num_cascades.glsl`
const uint max_num_trace_iters = 2048u;

////////// Outputs

out int num_collisions;

////////// Uniforms

layout(packed) uniform RandDirs {
	vec3 rand_dirs[max_num_trace_iters];
};

uniform uint
	workload_split_factor, num_traces_per_thread,
	max_num_ray_steps, max_point_height;

uniform float float_epsilon;

uniform usampler2DRect heightmap_sampler; // Note: other buffer types are too small for all the data needed.

/* Current scheme: casting the same rays per each position, with many positions per ray. A lot of branch divergence.
Alternate scheme: loop through positions in the shader, casting the same ray. That would require less repeated calculations. */

////////// Normal-finding code

#define NORMAL_IS_DUAL 0
#define NORMAL_IS_ABOVE_MAP 1
#define NORMAL_IS_BELOW_MAP 2
#define NORMAL_IS_ON_MAP 3

struct NormalData {
	vec3 normal;
	ivec3 flow;
	uint location_status;
};

NormalData compute_normal_data(const uvec3 origin) {
	////////// Computing the sign diffs and the flow

	uvec4 samples = textureGather(heightmap_sampler, origin.xz);

	// `sd` = sign diffs. Ordering of tl, tr, bl, br.
	ivec4 sd = sign(ivec4(samples.wzxy) - ivec4(origin.y));

	ivec3 flow = ivec3(
		(sd[2] - sd[3]) + (sd[0] - sd[1]),
		any(not(bvec4(sd))),
		(sd[0] - sd[2]) + (sd[1] - sd[3])
	);

	flow.xz = clamp(flow.xz, -1, 1);
	
	//////////

	NormalData normal_data = NormalData(normalize(flow), flow, NORMAL_IS_ON_MAP);

	if (flow == ivec3(0)) { // If no flow
		int top_left = sd[0];

		bool
			is_dual_normal = ((top_left < sd[2]) || (sd[1] < top_left)),
			is_below_map_if_not_dual_normal = (top_left == 1);

		normal_data.location_status = is_below_map_if_not_dual_normal ? NORMAL_IS_BELOW_MAP : NORMAL_IS_ABOVE_MAP;
		normal_data.location_status = is_dual_normal ? NORMAL_IS_DUAL : normal_data.location_status;
	}
	
	return normal_data;
}

////////// Collision code

bool ray_collides_with_heightmap(const vec3 dir, const uvec3 origin,
	const uvec3 map_size, const ivec3 flow, const bool is_dual_normal) {

	////////// https://www.shadertoy.com/view/3sKXDK, and http://www.cse.yorku.ca/~amana/research/grid.pdf

	ivec3 step_signs = ivec3(sign(dir));
	ivec3 actual_flow = is_dual_normal ? step_signs : flow;

	vec3 floating_origin = origin;
	floating_origin.xz -= vec2(equal(actual_flow.xz, ivec2(-1))) * float_epsilon;

	ivec3 curr_tile = ivec3(floor(floating_origin));

	vec3
		unit_step_size = abs(1.0f / dir),
		origin_minus_start = floating_origin - curr_tile;

	vec3 ray_length_components = unit_step_size * mix(
		origin_minus_start,
		1.0f - origin_minus_start,
		equal(step_signs, ivec3(1))
	);

	const ivec3 map_lower_bound = ivec3(0, -1, 0);
	ivec3 map_upper_bound = ivec3(map_size);
	map_upper_bound.xz--;

	//////////

	/* Note: for loop unrolling to work, it must be checked if
	that works with the maximum number of ray steps as well. */
	for (uint i = 0; i < max_num_ray_steps; i++) {
		uint x_and_y_min_index = uint(ray_length_components.x > ray_length_components.y);
		uint index_of_shortest = (ray_length_components[x_and_y_min_index] > ray_length_components[2]) ? 2 : x_and_y_min_index;

		curr_tile[index_of_shortest] += step_signs[index_of_shortest];
		ray_length_components[index_of_shortest] += unit_step_size[index_of_shortest];

		//////////

		if (curr_tile != clamp(curr_tile, map_lower_bound, map_upper_bound)) return false;
		else if (curr_tile.y < int(texelFetch(heightmap_sampler, curr_tile.xz).r)) return true;
	}

	return false;
}

void main(void) {
	////////// Computing the map size and the origin

	uint point_id = gl_VertexID / workload_split_factor;

	uvec3 map_size = uvec3(textureSize(heightmap_sampler), max_point_height).xzy;
	uint count_over_max_x = point_id / map_size.x;

	uvec3 origin = uvec3(
		point_id % map_size.x,
		count_over_max_x / map_size.z,
		count_over_max_x % map_size.z
	);

	////////// Computing the normal data, and exiting if below the map

	NormalData normal_data = compute_normal_data(origin);

	if (normal_data.location_status == NORMAL_IS_BELOW_MAP) {
		num_collisions = -1;
		return;
	}

	bool
		has_valid_normal = (normal_data.location_status == NORMAL_IS_ON_MAP),
		is_dual_normal = (normal_data.location_status == NORMAL_IS_DUAL);

	////////// Counting the number of collisions

	uint buffer_start = (gl_VertexID % workload_split_factor) * num_traces_per_thread;
	uint buffer_end = buffer_start + num_traces_per_thread;

	num_collisions = 0;

	for (uint i = buffer_start; i < buffer_end; i++) {
		vec3 rand_dir = rand_dirs[i]; // This is within a unit circle

		if (has_valid_normal && dot(normal_data.normal, rand_dir) < 0.0f)
			rand_dir = -rand_dir;

		num_collisions += int(ray_collides_with_heightmap(rand_dir, origin, map_size, normal_data.flow, is_dual_normal));
	}
}
