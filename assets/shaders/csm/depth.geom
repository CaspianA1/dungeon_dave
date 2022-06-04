#version 400 core

#include "num_cascades.geom"

#define VERTICES_PER_TRIANGLE 3u

layout(triangles, invocations = NUM_CASCADES) in;
layout(triangle_strip, max_vertices = VERTICES_PER_TRIANGLE) out;
	
// TODO: initialize this uniform
uniform mat4 light_view_projection_matrices[NUM_CASCADES];
	
void main(void) {
	mat4 light_matrix = light_view_projection_matrices[gl_InvocationID]; // TODO: find the type of matrix that this is

	for (uint i = 0; i < VERTICES_PER_TRIANGLE; i++) {
		gl_Position = light_matrix * gl_in[i].gl_Position;
		gl_Layer = gl_InvocationID; // This sets the cascade layer for the render invocation
		EmitVertex();
	}
	EndPrimitive();
}  
