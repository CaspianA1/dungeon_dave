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
#define DEBUG_FLOAT(var) printf(#var " = %ff\n", (GLdouble) (var))
#define DEBUG_VEC2(v) printf(#v " = {%ff, %ff}\n", (GLdouble) (v)[0], (GLdouble) (v)[1])
#define DEBUG_VEC3(v) printf(#v " = {%ff, %ff, %ff}\n", (GLdouble) (v)[0], (GLdouble) (v)[1], (GLdouble) (v)[2])
#define DEBUG_VEC4(v) printf(#v " = {%ff, %ff, %ff, %ff}\n", (GLdouble) (v)[0], (GLdouble) (v)[1], (GLdouble) (v)[2], (GLdouble) (v)[3]);
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
		const Uint8* const keys = SDL_GetKeyboardState(NULL);\
		\
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

////////// These are some general-purpose macros

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

////////// These macros pertain to window + rendering defaults

#define USE_VSYNC

#define USE_GAMMA_CORRECTION

#define USE_MULTISAMPLING
#define USE_POLYGON_ANTIALIASING

#define USE_ANISOTROPIC_FILTERING

// #define TRACK_MEMORY
// #define PRINT_SHADER_VALIDATION_LOG

//////////

/* Excluded: init_screen, deinit_screen, resize_window_if_needed,
set_triangle_fill_mode, application_should_exit, loop_application */

// Note: the drawer returns if the mouse should be visible.
void make_application(bool (*const drawer) (void* const, const Event* const),
	void* (*const init) (void), void (*const deinit) (void* const));

// Note: `x` and `y` are top-down here (making them technically `x` and `z`).
static inline byte sample_map_point(const byte* const map, const byte x, const byte z, const byte map_width) {
	return map[z * map_width + x];
}

static inline bool pos_out_of_overhead_map_bounds(const int16_t x,
	const int16_t z, const byte map_width, const byte map_height) {

	return (x < 0) || (z < 0) || (x >= (int16_t) map_width) || (z >= (int16_t) map_height);
}

const GLchar* get_GL_error(void);

FILE* open_file_safely(const GLchar* const path, const GLchar* const mode);

#endif
