#version 400 core

in vec2 translucent_fragment_quad_UV;

uniform bool drawing_translucent_quads;
uniform uint frame_index;
uniform float alpha_threshold;
uniform sampler2DArray alpha_test_sampler;

void main(void) {
	if (drawing_translucent_quads) {
		vec3 UV = vec3(translucent_fragment_quad_UV, frame_index);
		if (texture(alpha_test_sampler, UV).a < alpha_threshold) discard;
	}
}
