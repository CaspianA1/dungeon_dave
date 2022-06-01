#version 400 core

#include "common/utils.vert"

layout(location = 0) in vec3 vertex_pos_world_space;

uniform mat4 light_model_view_projection;

void main(void) {
	gl_Position = world_space_transformation(vertex_pos_world_space, light_model_view_projection);
}
