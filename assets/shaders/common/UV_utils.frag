#version 400 core

// From https://jorenjoestar.github.io/post/pixel_art_filtering/, under 'Inigo Quilez'
void pixelize_albedo_and_normal_UVs(
	const sampler2DArray albedo_sampler, const sampler2DArray normal_sampler,
	const vec2 bilinear_percents, const vec2 base_UV, out vec4 UVs) {

	UVs = vec4(base_UV, base_UV);

	vec4 texture_sizes = vec4(
		textureSize(albedo_sampler, 0).xy,
		textureSize(normal_sampler, 0).xy
	);

	vec4 pixel_positions = UVs * texture_sizes;

	vec4
		seams = floor(pixel_positions + 0.5f),

		dudvs = mix(
			fwidth(pixel_positions), vec4(1.0f),
			vec4(bilinear_percents, bilinear_percents)
		);

	pixel_positions = seams + clamp((pixel_positions - seams) / dudvs, -0.5f, 0.5f);
	UVs = pixel_positions / texture_sizes;
}

void get_albedo_and_normal(
	const sampler2DArray albedo_sampler,
	const sampler2DArray normal_sampler,
	const vec2 bilinear_percents, const vec3 base_UV,
	out vec4 albedo_color, out vec3 tangent_space_normal) {

	vec4 UVs;

	pixelize_albedo_and_normal_UVs(albedo_sampler,
		normal_sampler, bilinear_percents, base_UV.xy, UVs);

	albedo_color = texture(albedo_sampler, vec3(UVs.xy, base_UV.z));

	vec3 raw_tangent_space_normal = texture(normal_sampler, vec3(UVs.zw, base_UV.z)).xyz;
	tangent_space_normal = normalize(raw_tangent_space_normal * 2.0f - 1.0f);
}
