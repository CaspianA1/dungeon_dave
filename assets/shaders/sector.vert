#version 400 core

#include "common/shadow/shadow.vert"
#include "common/shared_params.glsl"
#include "common/world_shading.vert"

layout(location = 0) in vec3 vertex_pos_world_space;
layout(location = 1) in uint face_info_bits;

flat out uint face_id;

uniform sampler2DRect y_displacement_sampler;

const struct FaceAttribute {
	ivec2 uv_indices, uv_signs;
} face_attributes[5] = FaceAttribute[5](
	FaceAttribute(ivec2(0, 2), ivec2(1, 1)),
	FaceAttribute(ivec2(2, 1), ivec2(-1, -1)),
	FaceAttribute(ivec2(0, 1), ivec2(1, -1)),
	FaceAttribute(ivec2(2, 1), ivec2(1, -1)),
	FaceAttribute(ivec2(0, 1), ivec2(-1, -1))
);

float get_vertex_pos_y_displacement(const vec3 vertex_pos_world_space, const bool sector_is_dynamic) {
	/* TODO:
	- For the bottom vertices of sectors, extend them down so that they touch the ground.
		- If a sector has no vertical walls, generate that geometry in some way.
		- Otherwise, if a face is a vert face + based on the vertex ID, I can probably figure out if it's a bottom vertex.
			The current solution is just `vertex_pos_world_space.y == 0`, but that only works in a few cases.

	- For UV, wrap it, instead of stretching it for bottom-stretched dynamic sector vertices
	- Make sure that dynamic sectors that are next to each other have correct heights
	- Set up a system where I can validate the displacement areas first, and then modify their heights before rendering
	- Maybe support negative displacement
	- Handle frustum culling
	- Make a special shadow depth shader that incorporates displacement into the input vertices
	*/

	const ivec2 displacement_offsets[] = ivec2[](
		ivec2(0), ivec2(1, 1), ivec2(0, 1), ivec2(1, 0)
	);

	ivec2
		vertex_pos_xz = ivec2(vertex_pos_world_space.xz),
		max_extent = textureSize(y_displacement_sampler) - 1;

	float max_displacement = 0.0f;

	for (uint i = 0; i < displacement_offsets.length(); i++) {
		ivec2 sample_pos = min(vertex_pos_xz - displacement_offsets[i], max_extent);
		float displacement = texelFetch(y_displacement_sampler, sample_pos).r;
		max_displacement = max(max_displacement, displacement);
	}

	return max_displacement * float(sector_is_dynamic);
}

void main(void) {
	////////// Setting UV

	face_id = face_info_bits & 7u; // Extracting the first 3 bits
	FaceAttribute face_attribute = face_attributes[face_id];

	bool sector_is_dynamic = (face_info_bits >> 7u) == 1u; // Extracting bit 8

	UV.xy = face_attribute.uv_signs * vec2(
		vertex_pos_world_space[face_attribute.uv_indices.x],
		vertex_pos_world_space[face_attribute.uv_indices.y]
	);

	UV.z = ((face_info_bits & 127u) >> 3u); // Extracting bits 3 through 7

	////////// Vertex y-displacement

	fragment_pos_world_space = vertex_pos_world_space;
	fragment_pos_world_space.y += get_vertex_pos_y_displacement(fragment_pos_world_space, sector_is_dynamic);

	////////// Setting ambient_occlusion_UV, world_depth_value, and gl_Position

	ambient_occlusion_UV = get_ambient_occlusion_UV(fragment_pos_world_space);
	world_depth_value = get_world_depth_value(view, fragment_pos_world_space);
	gl_Position = view_projection * vec4(fragment_pos_world_space, 1.0f);
}
