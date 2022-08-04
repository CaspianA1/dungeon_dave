#ifndef UTILS_H
#define UTILS_H

#include "buffer_defs.h"
#include "event.h"

//////////

typedef enum {
	LoadSDL,
	LoadOpenGL,

	OpenFile,
	CreateSurface,
	CreateTexture,
	CreateFramebuffer,

	CreateShader,
	ParseIncludeDirectiveInShader,
	InitializeShaderUniform,

	InitializeGPUMemoryMapping,

	UseLevelHeightmap,
	TextureIDIsTooLarge
} FailureType;

////////// These macros pertain to debugging

#define GL_ERR_CHECK printf("GL error check: %s\n", get_GL_error());
#define SDL_ERR_CHECK printf("SDL error check: '%s'\n", SDL_GetError());

#define KEY_FLY SDL_SCANCODE_1
#define KEY_TOGGLE_WIREFRAME_MODE SDL_SCANCODE_2
#define KEY_PRINT_POSITION SDL_SCANCODE_3
#define KEY_PRINT_DIRECTION SDL_SCANCODE_4
#define KEY_PRINT_OPENGL_ERROR SDL_SCANCODE_5
#define KEY_PRINT_SDL_ERROR SDL_SCANCODE_6

#define DEBUG(var, format) printf(#var " = %" #format "\n", (var))
#define DEBUG_FLOAT(var) printf(#var " = %ff\n", (double) (var))
#define DEBUG_VEC2(v) printf(#v " = {%ff, %ff}\n", (double) (v)[0], (double) (v)[1])
#define DEBUG_VEC3(v) printf(#v " = {%ff, %ff, %ff}\n", (double) (v)[0], (double) (v)[1], (double) (v)[2])
#define DEBUG_VEC4(v) printf(#v " = {%ff, %ff, %ff, %ff}\n", (double) (v)[0], (double) (v)[1], (double) (v)[2], (double) (v)[3]);
#define DEBUG_RECT(r) printf(#r " = {%d, %d, %d, %d}\n", (r).x, (r).y, (r).w, (r).h)

#define DEBUG_BITS(num) do {\
	printf(#num " = ");\
	for (int16_t i = (sizeof(num) << 3) - 1; i >= 0; i--)\
		putchar((((num) >> i) & 1) + '0');\
	putchar('\n');\
} while (false)

#define TWEAK_REALTIME_VALUE(value_name, init_value, min_value, max_value, step, key_decr, key_incr, key_reset)\
	static GLfloat value_name = (init_value);\
	do {\
		const bool\
			incr = keys[SDL_SCANCODE_##key_incr],\
			decr = keys[SDL_SCANCODE_##key_decr],\
			reset = keys[SDL_SCANCODE_##key_reset];\
		\
		value_name = reset ? (init_value) : (value_name + (step) * incr - (step) * decr);\
		\
		if (value_name < (min_value)) value_name = (min_value);\
		else if (value_name > (max_value)) value_name = (max_value);\
		if (incr || decr || reset) DEBUG_FLOAT(value_name);\
	} while (false)

#define INIT_SHADER_BRANCH(shader, name, key) INIT_UNIFORM_VALUE(name, (shader), 1i, keys[SDL_SCANCODE_##key])

////////// These are some general-purpose macros

#define use_vertex_buffer(vertex_buffer) glBindBuffer(GL_ARRAY_BUFFER, (vertex_buffer))
#define ASSET_PATH(suffix) ("../assets/" suffix)

#define CHECK_BITMASK(bits, mask) (!!((bits) & (mask)))
#define ARRAY_LENGTH(array) (sizeof((array)) / sizeof(*(array)))

#define ON_FIRST_CALL(...) do {\
	static bool first_call = true;\
	if (first_call) {\
		__VA_ARGS__\
		first_call = false;\
	}\
} while (false)

#define FAIL(failure_type, format, ...) do {\
	fprintf(stderr, "Failed with error type '%s'. Reason: '"\
		format "'.\n", #failure_type, __VA_ARGS__);\
	exit(failure_type + 1);\
} while (false)

#define TIME(...)\
	const Uint32 before = SDL_GetTicks();\
	__VA_ARGS__\
	printf("Took %u milliseconds\n", SDL_GetTicks() - before);\

////////// Uniform-related macros

static inline GLint safely_get_uniform(const GLuint shader, const GLchar* const name) {
	const GLint id = glGetUniformLocation(shader, name);
	if (id == -1) FAIL(InitializeShaderUniform, "Uniform with the name of '%s' was not found in shader", name);
	return id;
}

#define INIT_UNIFORM(name, shader) name##_id = safely_get_uniform((shader), #name)

#define INIT_UNIFORM_VALUE(name, shader, type_prefix, ...)\
	glUniform##type_prefix(safely_get_uniform((shader), #name), __VA_ARGS__)

// TODO: try to remove this
#define INIT_UNIFORM_VALUE_FROM_VARIABLE_NAME(name, shader, type_prefix, ...)\
	glUniform##type_prefix(safely_get_uniform((shader), (name)), __VA_ARGS__)

#define UPDATE_UNIFORM(name, type_prefix, ...) glUniform##type_prefix(name##_id, __VA_ARGS__)

//////////

#define GENERIC_WITH_RENDER_STATE(setter, unsetter, state, inverse_state, ...) do {\
	setter((state)); __VA_ARGS__ unsetter((inverse_state));\
} while (false)

#define WITH_BINARY_RENDER_STATE(state, ...) GENERIC_WITH_RENDER_STATE(glEnable, glDisable, state, state, __VA_ARGS__)
#define WITHOUT_BINARY_RENDER_STATE(state, ...) GENERIC_WITH_RENDER_STATE(glDisable, glEnable, state, state, __VA_ARGS__)
#define WITH_RENDER_STATE(setter, state, inverse_state, ...) GENERIC_WITH_RENDER_STATE(setter, setter, state, inverse_state, __VA_ARGS__)

////////// These macros pertain to window + rendering defaults

#define USE_VSYNC

#define USE_GAMMA_CORRECTION

#define USE_MULTISAMPLING
#define USE_POLYGON_ANTIALIASING

#define USE_ANISOTROPIC_FILTERING

// #define TRACK_MEMORY

//////////

/* Excluded: init_screen, deinit_screen, resize_window_if_needed,
set_triangle_fill_mode, application_should_exit, loop_application */

void make_application(void (*const drawer) (void* const, const Event* const),
	void* (*const init) (void), void (*const deinit) (void* const));

// Note: `x` and `y` are top-down here (making them technically `x` and `z`).
byte sample_map_point(const byte* const map, const byte x, const byte y, const byte map_width);

const GLchar* get_GL_error(void);

void check_framebuffer_completeness(void);

FILE* open_file_safely(const GLchar* const path, const GLchar* const mode);

#endif
