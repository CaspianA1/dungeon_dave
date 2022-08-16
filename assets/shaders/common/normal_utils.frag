#version 400 core

#define MAKE_TANGENT_SPACE_NORMAL_GETTER(sampler_type, dims)\
	vec3 get_tangent_space_normal_##dims##D(const sampler_type s, const vec##dims UV) {\
		return normalize(texture(s, UV).rgb * 2.0f - 1.0f);\
	}\

MAKE_TANGENT_SPACE_NORMAL_GETTER(sampler2D, 2)
MAKE_TANGENT_SPACE_NORMAL_GETTER(sampler2DArray, 3)

#undef MAKE_TANGENT_SPACE_NORMAL_GETTER
