#version 330 core

in vec2 fragment_UV;

out vec4 color;

uniform uint frame_index;
uniform sampler2DArray frame_sampler;

void main(void) {
	color = texture(frame_sampler, vec3(fragment_UV, frame_index));
}
