#version 400 core

#include "shared_params.glsl"
#include "ambient_occlusion.vert"
#include "../shadow/shadow.vert"

out vec3
	fragment_pos_world_space, UV,
	camera_to_fragment_world_space,
	camera_to_fragment_tangent_space;

flat out mat3 fragment_tbn;

// Note: each vertex shader is expected to set `UV` independently.
void set_common_outputs(const vec3 vertex_pos_world_space, const mat3 tbn) {
	fragment_pos_world_space = vertex_pos_world_space;

	camera_to_fragment_world_space = camera_pos_world_space - vertex_pos_world_space; // For specular and volumetric lighting
	camera_to_fragment_tangent_space = transpose(tbn) * camera_to_fragment_world_space; // For parallax mapping
	fragment_tbn = tbn; // For lighting and parallax mapping
	ambient_occlusion_UV = get_ambient_occlusion_UV(vertex_pos_world_space); // For ambient occlusion
	world_depth_value = get_world_depth_value(view, fragment_pos_world_space); // For cascaded shadow mapping
	gl_Position = view_projection * vec4(vertex_pos_world_space, 1.0f); // For the fragment shader
}
