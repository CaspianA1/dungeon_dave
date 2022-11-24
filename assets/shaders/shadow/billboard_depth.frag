#version 400 core

in vec3 UV;

uniform float alpha_threshold;
uniform sampler2DArray albedo_sampler;

void main(void) {
	float // TODO: use an alpha buffer instead, for either alpha blending or alpha to coverage
		alpha = texture(albedo_sampler, UV).a,
		lod = textureQueryLod(albedo_sampler, UV.xy).x;

	/* Billboards that are far away tend to become overly
	transparent, so this rescales the alpha threshold to fix that. */
	const float almost_zero = 0.0001f;
	float rescaled_alpha_threshold = min(alpha_threshold / max(lod, almost_zero), 1.0f);

	if (alpha < rescaled_alpha_threshold) discard;
}
