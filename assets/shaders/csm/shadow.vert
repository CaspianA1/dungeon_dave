#version 400 core

uniform mat4 camera_view;

float get_world_depth_value(vec3 vertex_pos_world_space) {
    // TODO: see if I can make this more efficient (since I'm only using the Z component)
    return -(camera_view * vec4(vertex_pos_world_space, 1.0f)).z;
}
