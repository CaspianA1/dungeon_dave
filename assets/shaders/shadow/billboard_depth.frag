#version 400 core

in vec3 UV;

uniform float alpha_threshold;
uniform sampler2DArray diffuse_sampler;

void main(void) {
	// TODO: use an alpha buffer instead, for either alpha blending or alpha to coverage
	float alpha = texture(diffuse_sampler, UV).a;
	if (alpha < alpha_threshold) discard;
}
