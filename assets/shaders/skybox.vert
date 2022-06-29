#version 400 core

layout(location = 0) in ivec3 vertex_pos_world_space;

out vec3 UV;

uniform mat4 view_projection;

void main(void) {
	gl_Position = (view_projection * vec4(vertex_pos_world_space, 1.0f)).xyww; // With `.ww`, z is always set to 1.0f
	UV = vertex_pos_world_space;
	UV.x = -UV.x; // Without this, the x component of `UV` is reversed
}
