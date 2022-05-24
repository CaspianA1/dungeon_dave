#version 330 core

noperspective out vec3 fragment_UV;

uniform uint frame_index;
uniform vec2 weapon_corners[4];

void main(void) {
	gl_Position = vec4(weapon_corners[gl_VertexID], 0.0f, 1.0f);
	fragment_UV = vec3(gl_VertexID & 1, gl_VertexID < 2, frame_index);
}
