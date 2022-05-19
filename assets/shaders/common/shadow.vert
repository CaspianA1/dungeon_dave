#version 330 core

out vec3 fragment_pos_light_space;

uniform mat4 biased_light_model_view_projection;

vec3 world_to_light_space(vec3 vertex_pos_world_space) {
    return vec3(biased_light_model_view_projection * vec4(vertex_pos_world_space, 1.0f));
}
