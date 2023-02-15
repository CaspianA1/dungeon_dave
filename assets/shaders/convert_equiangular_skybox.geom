#version 400 core

#include "common/constants.geom"

layout(triangles, invocations = FACES_PER_CUBEMAP) in;
layout(triangle_strip, max_vertices = VERTICES_PER_TRIANGLE) out;

in vec3 translated_scaled_and_rotated_world_space[];

out vec3 rotated_fragment_pos_model_space;

uniform mat4 view_projections[FACES_PER_CUBEMAP];

void main(void) {
	for (uint i = 0; i < gl_in.length(); i++) {
		gl_Layer = gl_InvocationID;

		rotated_fragment_pos_model_space = gl_in[i].gl_Position.xyz;
		gl_Position = (mat3x4(view_projections[gl_Layer]) * translated_scaled_and_rotated_world_space[i]).xyww;

		EmitVertex();
	}
	EndPrimitive();
}
