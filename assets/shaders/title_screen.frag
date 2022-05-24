#version 330 core

noperspective in vec2 UV;

out vec3 color;

uniform float brightness;
uniform sampler2D texture_sampler;

void main(void) {
    color = texture(texture_sampler, UV).rgb * brightness;
}
