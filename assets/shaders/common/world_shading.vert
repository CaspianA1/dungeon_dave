#version 400 core

#include "shared_params.glsl"
#include "parallax_mapping.vert"
#include "ambient_occlusion.vert"
#include "../shadow/shadow.vert"

out vec3 fragment_pos_world_space, UV;
flat out mat3 fragment_tbn;

// Note: each vertex shader is expected to set `UV` independently.
void set_common_outputs(const vec3 vertex_pos_world_space, const mat3 tbn) {
    fragment_pos_world_space = vertex_pos_world_space;

    // For parallax mapping
    camera_to_fragment_tangent_space = get_vector_to_vertex_in_tangent_space(
		camera_pos_world_space, vertex_pos_world_space, tbn
	);

    // For lighting and parallax mapping
    fragment_tbn = tbn;

    // For ambient occlusion
    ambient_occlusion_UV = get_ambient_occlusion_UV(vertex_pos_world_space);

    // For cascaded shadow mapping
    world_depth_value = get_world_depth_value(view, fragment_pos_world_space);

    // For the fragment shader
    gl_Position = view_projection * vec4(vertex_pos_world_space, 1.0f);
}
