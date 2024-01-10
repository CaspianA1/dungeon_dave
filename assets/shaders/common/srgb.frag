#version 400 core

float convert_srgb_to_linear(const float srgb) {
	/* The sRGB -> linear mapping is based on function from the bottom of here:
	https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml. */

	float
		linear_for_below = srgb / 12.92f,
		linear_for_above = pow((srgb + 0.055f) / 1.055f, 2.4f);

	return mix(linear_for_above, linear_for_below, float(srgb <= 0.04045f));
}
