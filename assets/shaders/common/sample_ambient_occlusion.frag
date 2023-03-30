#version 400 core

uniform sampler3D ambient_occlusion_sampler;

/* The tricubic filtering code is based on the bicubic filtering code from here:
https://stackoverflow.com/questions/13501081/efficient-bicubic-filtering-code-in-glsl. */

vec4 cubic(const float v) {
	vec3 n = vec3(1.0f, 2.0f, 3.0f) - v;
	vec3 s = n * n * n;

	float
		a = s.y - (4.0f * s.x),
		b = s.z - (4.0f * s.y) + (6.0f * s.x);

	return vec4(s.x, b, a, 6.0f - s.x - a - b) / 6.0f;
}

// Tricubic filtering reads some values under the heightmap, which makes it a bit darker.
float texture_tricubic_single_channel(const sampler3D texture_sampler, const vec3 fragment_pos_world_space) {
	vec3
		texel_size = 1.0f / textureSize(texture_sampler, 0),
		UV = fragment_pos_world_space.xzy;

	vec3 UV_fraction = fract(UV);
	UV -= UV_fraction; // Flooring it

	/* Note: to get rid of the aliasing, simply set the UV fraction to some
	constant, and don't floor the UV (but that causes other problems as well). */

	vec4 x_cubic = cubic(UV_fraction.x), y_cubic = cubic(UV_fraction.y), z_cubic = cubic(UV_fraction.z);

	//////////

	const vec2 c_texel_offset = vec2(-0.5f, 1.5f);

	vec4 c = UV.xxyy + c_texel_offset.xyxy;
	vec2 c_extra = UV.z + c_texel_offset;

	//////////

	vec4 s = vec4(x_cubic.xy + x_cubic.zw, y_cubic.xy + y_cubic.zw);
	vec4 sample_offset = (c + vec4(x_cubic.zw, y_cubic.zw) / s) * texel_size.xxyy;

	vec2 s_extra = z_cubic.xy + z_cubic.zw;
	vec2 sample_offset_extra = (c_extra + vec2(z_cubic.zw) / s_extra) * texel_size.z;

	//////////

	#define SAMPLE(gh, i) texture(texture_sampler, vec3(sample_offset.gh, sample_offset_extra.i)).r

	vec4 lerped_x = mix(
		vec4(SAMPLE(yw, x), SAMPLE(yw, y), SAMPLE(yz, x), SAMPLE(yz, y)),
		vec4(SAMPLE(xw, x), SAMPLE(xw, y), SAMPLE(xz, x), SAMPLE(xz, y)),
		s.x / (s.x + s.y)
	);

	#undef SAMPLE

	vec2 lerped_y = mix(lerped_x.xy, lerped_x.zw, s.z / (s.z + s.w));
	return mix(lerped_y.y, lerped_y.x, s_extra.x / (s_extra.x + s_extra.y));
}

float get_ambient_strength(const vec3 fragment_pos_world_space) {
	float ao_strength = ambient_occlusion.strength * texture_tricubic_single_channel(ambient_occlusion_sampler, fragment_pos_world_space);

	/* The sRGB -> linear mapping is based on function from the bottom of here:
	https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml.
	It's done manually because there are no sRGB formats for 8-bit textures.
	TODO: move this calculation to the CPU. */

	float
		linear_for_below = ao_strength / 12.92f,
		linear_for_above = pow((ao_strength + 0.055f) / 1.055f, 2.5f);

	return mix(linear_for_above, linear_for_below, float(ao_strength <= 0.04045f));
}
