#version 400 core

uniform mat4 camera_view;

out float world_depth_value;

float get_world_depth_value(vec3 vertex_pos_world_space) {
    return -(camera_view * vec4(vertex_pos_world_space, 1.0f)).z;
}
