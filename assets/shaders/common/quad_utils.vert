#version 400 core

/* The code here is for things that are drawn as quad triangle
strips, like billboards, the weapon, or the title screen.

`quad_corners` will typically be indexed into
by gl_VertexID in order to generate a position. */

#define CORNERS_PER_QUAD 4u

const vec2 quad_corners[CORNERS_PER_QUAD] = vec2[](
	vec2(-1.0f, -1.0f), vec2(1.0f, -1.0f),
	vec2(-1.0f, 1.0f), vec2(1.0f, 1.0f)
);

vec2 get_quad_UV(void) {
	return vec2(gl_VertexID & 1, gl_VertexID < 2);
}
