#ifndef SHADER_CODE_H
#define SHADER_CODE_H

#define SHADER_PAIR_DEF(name) *const name##_vertex_shader, *const name##_fragment_shader

extern const GLchar
	SHADER_PAIR_DEF(sector),
	SHADER_PAIR_DEF(billboard), SHADER_PAIR_DEF(skybox),
	SHADER_PAIR_DEF(weapon), SHADER_PAIR_DEF(depth);

#undef SHADER_DEF

#endif
