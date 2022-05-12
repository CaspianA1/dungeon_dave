#version 330 core

layout(location = 0) in vec3 vertex_pos_world_space;

uniform mat4 light_model_view_projection;

void main(void) {
	gl_Position = light_model_view_projection * vec4(vertex_pos_world_space, 1.0f);
}
