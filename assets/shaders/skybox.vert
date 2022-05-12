#version 330 core

layout(location = 0) in ivec3 vertex_pos_world_space;

out vec3 UV_3D;

uniform mat4 model_view_projection;

void main(void) {
	gl_Position = (model_view_projection * vec4(vertex_pos_world_space, 1.0f)).xyww;
	UV_3D = vertex_pos_world_space;
	UV_3D.x = -UV_3D.x; // Without this, the X component of the UV is reversed
}
