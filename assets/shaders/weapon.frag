#version 330 core

in vec3 fragment_UV;

out vec4 color;

uniform sampler2DArray frame_sampler;

void main(void) {
	color = texture(frame_sampler, fragment_UV);
}
