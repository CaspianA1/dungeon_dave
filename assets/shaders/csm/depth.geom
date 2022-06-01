#version 400 core

#define VERTICES_PER_TRIANGLE 3u

#include "csm.common"

layout(triangles, invocations = NUM_CASCADE_LAYERS) in;
layout(triangle_strip, max_vertices = VERTICES_PER_TRIANGLE) out;
	
// TODO: initialize this uniform
uniform mat4 light_space_matrices[NUM_CASCADE_LAYERS];
	
void main(void) {
	mat4 light_space_matrix = light_space_matrices[gl_InvocationID]; // TODO: find the type of matrix that this is

	for (uint i = 0; i < VERTICES_PER_TRIANGLE; i++) {
		gl_Position = light_space_matrix * gl_in[i].gl_Position;
		gl_Layer = gl_InvocationID; // This sets the cascade layer for the render invocation
		EmitVertex();
	}
	EndPrimitive();
}  
