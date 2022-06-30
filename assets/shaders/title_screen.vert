#version 400 core

#include "quad.vert"

noperspective out vec2 UV;

void main(void) {
	UV = get_quad_UV();
	gl_Position = vec4(quad_corners[gl_VertexID], 0.0f, 1.0f);
}
