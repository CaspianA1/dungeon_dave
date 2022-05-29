#version 330 core

#include "../common/utils.vert"

layout(location = 0) in vec3 vertex_pos_world_space;

uniform mat4 model; // No idea if this is right

void main(void) {
    gl_Position = world_space_transformation(vertex_pos_world_space, model);
}
