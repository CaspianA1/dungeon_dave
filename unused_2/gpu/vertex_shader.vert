#version 300

uniform int ground_size, screen_width;
uniform double straight_dist;
uniform vec2 pos;

in int screen_x;
in double one_over_cos_beta;
in vec2 dir;

out double actual_dist;
out vec2 hit, tex_offset;

void main() {
	actual_dist = straight_dist * one_over_cos_beta;
	hit = dir * vec2(actual_dist) + pos;
	tex_offset = vec2(ground_size) * fract(hit);
}
