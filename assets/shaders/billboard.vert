#version 400 core

#include "csm/shadow.vert"

layout(location = 0) in uint texture_id;
layout(location = 1) in vec2 billboard_size_world_space;
layout(location = 2) in vec3 billboard_center_world_space;

out vec3 UV, fragment_pos_world_space;

uniform vec2 right_xz_world_space;
uniform mat4 model_view_projection;

const vec2 vertices_model_space[4] = vec2[4](
	vec2(-0.5f, -0.5f), vec2(0.5f, -0.5f),
	vec2(-0.5f, 0.5f), vec2(0.5f, 0.5f)
);

void main(void) {
	vec2 vertex_pos_model_space = vertices_model_space[gl_VertexID];
	vec2 upscaled_vertex_pos_world_space = vertex_pos_model_space * billboard_size_world_space;

	vec3 vertex_pos_world_space = billboard_center_world_space
		+ upscaled_vertex_pos_world_space.x * vec3(right_xz_world_space, 0.0f).xzy
		+ vec3(0.0f, upscaled_vertex_pos_world_space.y, 0.0f);

	fragment_pos_world_space = vertex_pos_world_space;
	world_depth_value = get_world_depth_value(vertex_pos_world_space);
	gl_Position = model_view_projection * vec4(vertex_pos_world_space, 1.0f);

	UV.xy = vec2(vertex_pos_model_space.x, -vertex_pos_model_space.y) + 0.5f;
	UV.z = texture_id;
}
