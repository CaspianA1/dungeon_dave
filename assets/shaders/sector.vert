#version 400 core

#include "common/world_shading.vert"

layout(location = 0) in ivec3 vertex_pos_world_space;
layout(location = 1) in uint face_info_bits;

const struct FaceAttribute {
	vec3 tangent, normal;
} face_attributes[5] = FaceAttribute[5]( // Flat, right, bottom, left, top
	FaceAttribute(vec3(-1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f)),
	FaceAttribute(vec3(0.0f, 0.0f, 1.0f), vec3(1.0f, 0.0f, 0.0f)),
	FaceAttribute(vec3(-1.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f)),
	FaceAttribute(vec3(0.0f, 0.0f, -1.0f), vec3(-1.0f, 0.0f, 0.0f)),
	FaceAttribute(vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, -1.0f))
);

void main(void) {
	////////// Setting `material_index`, `bilinear_percents_index`, `UV`, and the common outputs

	uint face_id = face_info_bits & 7u; // Extracting the first 3 bits
	FaceAttribute face_attribute = face_attributes[face_id];

	mat3 tbn = mat3(
		face_attribute.tangent,
		cross(face_attribute.tangent, face_attribute.normal),
		face_attribute.normal
	);

	// Shifting over to get the material index/texture id
	material_index = face_info_bits >> 3u;
	bilinear_percents_index = 0u;

	UV = vec3(
		-vertex_pos_world_space * mat2x3(tbn),
		material_index
	);

	set_common_outputs(vertex_pos_world_space, tbn);
}
