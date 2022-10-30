#version 400 core

// Note: the fourth component of this is an inverse heightmap value.
#define MAKE_TANGENT_SPACE_NORMAL_GETTER(sampler_type, dims)\
	vec4 get_tangent_space_normal_##dims##D(const sampler_type s, const vec##dims UV) {\
		vec4 normal_and_inv_height = texture(s, UV);\
		return vec4(normalize(normal_and_inv_height.rgb * 2.0f - 1.0f), normal_and_inv_height.a);\
	}\

MAKE_TANGENT_SPACE_NORMAL_GETTER(sampler2D, 2)
MAKE_TANGENT_SPACE_NORMAL_GETTER(sampler2DArray, 3)

#undef MAKE_TANGENT_SPACE_NORMAL_GETTER
