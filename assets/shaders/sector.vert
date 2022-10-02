#version 400 core

#include "shadow/shadow.vert"
#include "common/shared_params.glsl"
#include "common/world_shading.vert"
#include "common/parallax_mapping.vert"

layout(location = 0) in vec3 vertex_pos_world_space;
layout(location = 1) in uint face_info_bits;

flat out mat3 tbn;

const struct FaceAttribute {
	ivec2 uv_indices, uv_signs;
	vec3 tangent, normal;
} face_attributes[5] = FaceAttribute[5]( // Flat, right, bottom, left, top
	FaceAttribute(ivec2(0, 2), ivec2(1, 1), vec3(-1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f)),
	FaceAttribute(ivec2(2, 1), ivec2(-1, -1), vec3(0.0f, 0.0f, 1.0f), vec3(1.0f, 0.0f, 0.0f)),
	FaceAttribute(ivec2(0, 1), ivec2(1, -1), vec3(-1.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f)),
	FaceAttribute(ivec2(2, 1), ivec2(1, -1), vec3(0.0f, 0.0f, -1.0f), vec3(-1.0f, 0.0f, 0.0f)),
	FaceAttribute(ivec2(0, 1), ivec2(-1, -1), vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, -1.0f))
);

void main(void) {
	////////// Setting UV and camera_fragment_delta_tangent_space

	uint face_id = face_info_bits & 7u; // Extracting the first 3 bits
	FaceAttribute face_attribute = face_attributes[face_id];

	UV.xy = face_attribute.uv_signs * vec2(
		vertex_pos_world_space[face_attribute.uv_indices.x],
		vertex_pos_world_space[face_attribute.uv_indices.y]
	);

	UV.z = face_info_bits >> 3u; // Shifting over to get the texture id

	tbn = mat3(
		face_attribute.tangent,
		cross(face_attribute.tangent, face_attribute.normal),
		face_attribute.normal
	);

	camera_to_fragment_tangent_space = get_vector_to_vertex_in_tangent_space(
		camera_pos_world_space, vertex_pos_world_space, tbn
	);

	////////// Setting ambient_occlusion_UV, world_depth_value, fragment_pos_world_space, and gl_Position

	ambient_occlusion_UV = get_ambient_occlusion_UV(vertex_pos_world_space);
	world_depth_value = get_world_depth_value(view, vertex_pos_world_space);
	fragment_pos_world_space = vertex_pos_world_space;
	gl_Position = view_projection * vec4(vertex_pos_world_space, 1.0f);
}
