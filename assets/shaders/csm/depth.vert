#version 400 core

layout(location = 0) in vec3 vertex_pos_world_space;

void main(void) {
    gl_Position = vec4(vertex_pos_world_space, 1.0f);
}