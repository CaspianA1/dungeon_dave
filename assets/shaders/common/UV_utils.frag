#version 400 core

// https://jorenjoestar.github.io/post/pixel_art_filtering/, under 'Inigo Quilez'
void adjust_UV_for_pixel_art_filtering(const float bilinear_percent, const vec2 texture_size, inout vec2 UV) {
	vec2 pixel = UV * texture_size;

	vec2
		seam = floor(pixel + 0.5f),
		dudv = mix(fwidth(pixel), vec2(1.0f), bilinear_percent);

	pixel = seam + clamp((pixel - seam) / dudv, -0.5f, 0.5f);
	UV = pixel / texture_size;
}
