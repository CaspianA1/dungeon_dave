#ifndef DEBUG_MACRO_UTILS_H
#define DEBUG_MACRO_UTILS_H

// TODO: remove this file eventually

#include <stdio.h> // For IO functions
#include <stdbool.h> // For `bool`
#include "utils/opengl_wrappers.h" // For OpenGL types + `get_GL_error`
#include "openal/al.h" // For `alGetError`, and various OpenAL #defines
#include "openal/alc.h" // For `alcGetError`, and various ALC #defines
#include "utils/sdl_include.h" // For `SDL_Scancode` values, `Uint32`, and `SDL_GetTicks`

static inline const GLchar* get_AL_error(void) {
	switch (alGetError()) {
		#define ERROR_CASE(error) case AL_##error: return #error;

		ERROR_CASE(NO_ERROR);
		ERROR_CASE(INVALID_NAME);
		ERROR_CASE(INVALID_ENUM);
		ERROR_CASE(INVALID_VALUE);
		ERROR_CASE(INVALID_OPERATION);
		ERROR_CASE(OUT_OF_MEMORY);

		#undef ERROR_CASE
	}
	return "Unknown error";
}

static inline const GLchar* get_ALC_error(void) {
	ALCcontext* const context = alcGetCurrentContext();
	ALCdevice* const device = alcGetContextsDevice(context);

	switch (alcGetError(device)) {
		#define ERROR_CASE(error) case ALC_##error: return #error;

		ERROR_CASE(NO_ERROR);
		ERROR_CASE(INVALID_DEVICE);
		ERROR_CASE(INVALID_CONTEXT);
		ERROR_CASE(INVALID_ENUM);
		ERROR_CASE(INVALID_VALUE);
		ERROR_CASE(OUT_OF_MEMORY);

		#undef ERROR_CASE
	}
	return "Unknown error";
}

//////////

#define KEY_FLY SDL_SCANCODE_1
#define KEY_TOGGLE_WIREFRAME_MODE SDL_SCANCODE_2
#define KEY_PRINT_POSITION SDL_SCANCODE_3
#define KEY_PRINT_DIRECTION SDL_SCANCODE_4

#define KEY_PRINT_SDL_ERROR SDL_SCANCODE_5
#define KEY_PRINT_OPENGL_ERROR SDL_SCANCODE_6
#define KEY_PRINT_AL_ERROR SDL_SCANCODE_7
#define KEY_PRINT_ALC_ERROR SDL_SCANCODE_8

//////////

#define SDL_ERR_CHECK printf("SDL error check: '%s'\n", SDL_GetError());
#define GL_ERR_CHECK printf("GL error check: %s\n", get_GL_error());
#define AL_ERR_CHECK printf("AL error check: %s\n", get_AL_error());
#define ALC_ERR_CHECK printf("ALC error check: %s\n", get_ALC_error());

//////////

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

#define TIME(...)\
	const Uint32 before = SDL_GetTicks();\
	__VA_ARGS__\
	printf("Took %u milliseconds\n", SDL_GetTicks() - before);\

#endif
