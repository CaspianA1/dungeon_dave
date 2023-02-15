#version 400 core

#include "common/constants.geom"

layout(location = 0) in vec3 vertex;

out vec3 translated_scaled_and_rotated_world_space;

uniform mat4 translation_scaling_matrix, rotation_matrices[FACES_PER_CUBEMAP];

void main(void) {
	////////// http://mathproofs.blogspot.com/2005/07/mapping-cube-to-sphere.html

	vec3 orig = vertex;

	vec3 squared = orig * orig;
	vec3 left = squared.yxx, right = squared.zzy;
	vec3 products = left * right, sums = left + right;

	const float one_third = 1.0f / 3.0f;
	vec3 sphere_vertex = vertex * sqrt(1.0f - (sums * 0.5f) + (products * one_third));

	//////////

	gl_Position = vec4(mat3x3(rotation_matrices[gl_InstanceID]) * sphere_vertex, 1.0f);
	translated_scaled_and_rotated_world_space = mat4x3(translation_scaling_matrix) * gl_Position;
}
