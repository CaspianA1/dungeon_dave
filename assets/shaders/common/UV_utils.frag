#version 400 core

// https://jorenjoestar.github.io/post/pixel_art_filtering/, under 'Inigo Quilez'
void adjust_UV_for_pixel_art_filtering(const float bilinear_percent, const vec2 texture_size, inout vec2 UV) {
	vec2 pixel_pos = UV * texture_size;

	vec2
		seam = floor(pixel_pos + 0.5f),
		dudv = mix(fwidth(pixel_pos), vec2(1.0f), bilinear_percent);

	pixel_pos = seam + clamp((pixel_pos - seam) / dudv, -0.5f, 0.5f);
	UV = pixel_pos / texture_size;
}
