#version 400 core

#include "../common/shared_params.glsl"

#define VERTICES_PER_TRIANGLE 3u

layout(triangles, invocations = NUM_CASCADES) in;
layout(triangle_strip, max_vertices = VERTICES_PER_TRIANGLE) out;

#define DEFINE_GEOMETRY_STAGE_OF_DEPTH_SHADER(unique_io, specifying_unique_output)\
\
unique_io \
void main(void) {\
	mat4 light_view_projection_matrix = light_view_projection_matrices[gl_InvocationID];\
	\
	for (uint i = 0; i < VERTICES_PER_TRIANGLE; i++) {\
		\ // Need to re-write to these since their values are undefined after invoking `EmitVertex`
		gl_Layer = gl_InvocationID;\ // This sets the cascade layer for the rendering invocation
		gl_Position = light_view_projection_matrix * gl_in[i].gl_Position;\
		specifying_unique_output\
		EmitVertex();\
	}\
	EndPrimitive();\
}
