#version 400 core

in vec3 UV;

uniform sampler2DArray albedo_sampler;

void main(void) {
	if (texture(albedo_sampler, UV).a != 1.0f) discard;
}
