#version 400 core

uniform sampler3D ambient_occlusion_sampler;

/* The tricubic filtering code is based on the bicubic filtering code from here:
https://stackoverflow.com/questions/13501081/efficient-bicubic-filtering-code-in-glsl. */

vec4 cubic(const float v) {
	vec3 n = vec3(1.0f, 2.0f, 3.0f) - v;
	vec3 s = n * n * n;

	float
		x = s.x,
		y = s.y - 4.0f * s.x,
		z = s.z - 4.0f * s.y + 6.0f * s.x;

	return vec4(x, y, z, 6.0f - x - y - z) / 6.0f;
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

	//////////

	const vec2 c_texel_offset = vec2(-0.5f, 1.5f);

	vec4 c = UV.xxyy + c_texel_offset.xyxy;
	vec2 c_extra = UV.z + c_texel_offset;

	//////////

	vec4 x_cubic = cubic(UV_fraction.x), y_cubic = cubic(UV_fraction.y), z_cubic = cubic(UV_fraction.z);

	vec4 s = vec4(x_cubic.xz + x_cubic.yw, y_cubic.xz + y_cubic.yw);
	vec4 sample_offset = (c + vec4(x_cubic.yw, y_cubic.yw) / s) * texel_size.xxyy;

	vec2 s_extra = z_cubic.xz + z_cubic.yw;
	vec2 sample_offset_extra = (c_extra + vec2(z_cubic.yw) / s_extra) * texel_size.z;

	//////////

	#define SAMPLE(gh, i) texture(texture_sampler, vec3(sample_offset.gh, sample_offset_extra.i)).r

	vec4
		lower_samples = vec4(SAMPLE(xz, x), SAMPLE(yz, x), SAMPLE(xw, x), SAMPLE(yw, x)),
		higher_samples = vec4(SAMPLE(xz, y), SAMPLE(yz, y), SAMPLE(xw, y), SAMPLE(yw, y));
	
	#undef SAMPLE
	
	vec3 lerp_weights = vec3(
		s.x / (s.x + s.y),
		s.z / (s.z + s.w),
		s_extra.x / (s_extra.x + s_extra.y)
	);

	vec4 lerped_x = mix(
		vec4(lower_samples.wy, higher_samples.wy),
		vec4(lower_samples.zx, higher_samples.zx),
		lerp_weights.x
	);

	vec2 lerped_y = mix(lerped_x.xz, lerped_x.yw, lerp_weights.y);
	return mix(lerped_y.y, lerped_y.x, lerp_weights.z);
}

float get_ambient_strength(const vec3 fragment_pos_world_space) {
	float raw_ao_amount = texture_tricubic_single_channel(ambient_occlusion_sampler, fragment_pos_world_space);
	float raw_ao_strength = ambient_occlusion.strength * raw_ao_amount;

	/* The sRGB -> linear mapping is based on function from the bottom of here:
	https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml.
	It's done manually because there are no sRGB formats for 8-bit textures.
	TODO: move this calculation to the CPU. */

	float
		linear_for_below = raw_ao_strength / 12.92f,
		linear_for_above = pow((raw_ao_strength + 0.055f) / 1.055f, 2.5f);

	return mix(linear_for_above, linear_for_below, float(raw_ao_strength <= 0.04045f));
}
