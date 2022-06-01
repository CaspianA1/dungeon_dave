#version 400 core

vec4 world_space_transformation(vec3 vertex_pos_world_space, mat4 transformer) {
    return transformer * vec4(vertex_pos_world_space, 1.0f);
}
