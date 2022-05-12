#version 330 core

uniform vec2 weapon_corners[4];

out vec2 fragment_UV;

void main(void) {
	gl_Position = vec4(weapon_corners[gl_VertexID], 0.0f, 1.0f);
	fragment_UV = vec2(gl_VertexID & 1, gl_VertexID < 2);
}
