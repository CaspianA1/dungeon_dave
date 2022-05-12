#version 330 core

layout(location = 0) in vec3 vertex_pos_world_space;
layout(location = 1) in uint face_info_bits;

flat out uint face_id;

out vec3 UV, fragment_pos_light_space, fragment_pos_world_space;

uniform mat4 model_view_projection, biased_light_model_view_projection;

const struct FaceAttribute {
	ivec2 uv_indices, uv_signs;
} face_attributes[5] = FaceAttribute[5](
	FaceAttribute(ivec2(0, 2), ivec2(1, 1)),
	FaceAttribute(ivec2(2, 1), ivec2(-1, -1)),
	FaceAttribute(ivec2(0, 1), ivec2(1, -1)),
	FaceAttribute(ivec2(2, 1), ivec2(1, -1)),
	FaceAttribute(ivec2(0, 1), ivec2(-1, -1))
);

void main(void) {
 	////////// Setting face_normal and UV

	face_id = face_info_bits & 7u;
	FaceAttribute face_attribute = face_attributes[face_id];

	vec2 UV_xy = face_attribute.uv_signs * vec2(
		vertex_pos_world_space[face_attribute.uv_indices.x],
		vertex_pos_world_space[face_attribute.uv_indices.y]
	);

	UV = vec3(UV_xy, face_info_bits >> 3u);

	////////// Setting fragment_pos_world_space, fragment_pos_light_space, and gl_Position

	fragment_pos_world_space = vertex_pos_world_space;
	vec4 vertex_pos_world_space_4D = vec4(vertex_pos_world_space, 1.0f);

	fragment_pos_light_space = vec3(biased_light_model_view_projection * vertex_pos_world_space_4D);
	gl_Position = model_view_projection * vertex_pos_world_space_4D;
}
