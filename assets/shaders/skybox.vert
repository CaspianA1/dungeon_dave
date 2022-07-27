#version 400 core

out vec3 UV;

uniform mat4 view_projection; // TODO: share this in some way with the uniform buffer

// https://stackoverflow.com/questions/28375338/cube-using-single-gl-triangle-strip
const ivec3 vertices[] = ivec3[](
	ivec3(-1, 1, 1),  ivec3(1, 1, 1),    ivec3(-1, -1, 1), ivec3(1, -1, 1),
	ivec3(1, -1, -1), ivec3(1, 1, 1),    ivec3(1, 1, -1),  ivec3(-1, 1, 1),  ivec3(-1, 1, -1),
	ivec3(-1, -1, 1), ivec3(-1, -1, -1), ivec3(1, -1, -1), ivec3(-1, 1, -1), ivec3(1, 1, -1)
);

void main(void) {
	vec3 vertex_pos_world_space = vertices[gl_VertexID];
	gl_Position = view_projection * vec4(vertex_pos_world_space, 1.0f);
	UV = vertex_pos_world_space;
	UV.x = -UV.x; // Without this, the x component of `UV` is reversed
}
