#version 400 core

#include "num_cascades.geom"

#define VERTICES_PER_TRIANGLE 3u

layout(triangles, invocations = NUM_CASCADES) in;
layout(triangle_strip, max_vertices = VERTICES_PER_TRIANGLE) out;

in vec2 translucent_vertex_quad_UV[];
out vec2 translucent_fragment_quad_UV;

uniform mat4 light_view_projection_matrices[NUM_CASCADES];

void main(void) {
	mat4 light_view_projection_matrix = light_view_projection_matrices[gl_InvocationID];

	for (uint i = 0; i < VERTICES_PER_TRIANGLE; i++) {
		// Need to re-write to these since their values are undefined after invoking `EmitVertex`
		gl_Layer = gl_InvocationID; // This sets the cascade layer for the rendering invocation
		gl_Position = light_view_projection_matrix * gl_in[i].gl_Position;
		translucent_fragment_quad_UV = translucent_vertex_quad_UV[i];

		EmitVertex();
	}
	EndPrimitive();
}

#undef VERTICES_PER_TRIANGLE
