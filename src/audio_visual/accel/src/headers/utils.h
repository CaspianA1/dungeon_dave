#ifndef UTILS_H
#define UTILS_H

#include <SDL2/SDL.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#include <cglm/cglm.h>
#pragma GCC diagnostic pop

#include "buffer_defs.h"

////////// These macros pertain to debugging

#define GL_ERR_CHECK printf("GL error check: %s\n", get_gl_error());
#define SDL_ERR_CHECK printf("SDL error check: '%s'\n", SDL_GetError());

#define KEY_FLY SDL_SCANCODE_1
#define KEY_TOGGLE_WIREFRAME_MODE SDL_SCANCODE_2
#define KEY_PRINT_OPENGL_ERROR SDL_SCANCODE_3
#define KEY_PRINT_SDL_ERROR SDL_SCANCODE_4
#define KEY_PRINT_POSITION SDL_SCANCODE_5
#define KEY_PRINT_DIRECTION SDL_SCANCODE_6
#define KEY_PRINT_UP SDL_SCANCODE_7

#define DEBUG(var, format) printf(#var " = %" #format "\n", (var))
#define DEBUG_FLOAT(var) printf(#var " = %ff\n", (double) (var))
#define DEBUG_VEC2(v) printf(#v " = {%ff, %ff}\n", (double) (v)[0], (double) (v)[1])
#define DEBUG_VEC3(v) printf(#v " = {%ff, %ff, %ff}\n", (double) (v)[0], (double) (v)[1], (double) (v)[2])

#define DEBUG_BITS(num) do {\
	printf(#num " = ");\
	for (int16_t i = (sizeof(num) << 3) - 1; i >= 0; i--)\
		putchar((((num) >> i) & 1) + '0');\
	putchar('\n');\
} while (0)

#define TWEAK_REALTIME_VALUE(value_name, init_value, min_value, max_value, step, key_decr, key_incr, key_reset)\
	static GLfloat value_name = init_value;\
	do {\
		const bool\
			incr = keys[SDL_SCANCODE_##key_incr],\
			decr = keys[SDL_SCANCODE_##key_decr],\
			reset = keys[SDL_SCANCODE_##key_reset];\
		\
		value_name = reset ? init_value : (value_name + step * incr - step * decr);\
		\
		if (value_name < min_value) value_name = min_value;\
		else if (value_name > max_value) value_name = max_value;\
		if (incr || decr || reset) DEBUG_FLOAT(value_name);\
	} while (0)

#define MAKE_SHADER_BRANCH(shader, key) INIT_UNIFORM_VALUE(branch, (shader), 1i, keys[SDL_SCANCODE_##key]);

////////// These are some general-purpose macros used in all demos

#define inlinable static inline

#define bit_is_set(bits, mask) ((bits) & (mask))
#define set_bit(bits, mask) ((bits) |= (mask))

#define INIT_UNIFORM(name, shader) name##_id = glGetUniformLocation((shader), #name)

#define INIT_UNIFORM_VALUE(name, shader, type_prefix, ...)\
	glUniform##type_prefix(glGetUniformLocation(shader, #name), __VA_ARGS__)

#define INIT_UNIFORM_VALUE_FROM_VARIABLE_NAME(name, shader, type_prefix, ...)\
	glUniform##type_prefix(glGetUniformLocation(shader, name), __VA_ARGS__)

#define UPDATE_UNIFORM(name, type_prefix, ...) glUniform##type_prefix(name##_id, __VA_ARGS__)

////////// These macros are for snake-case names of OpenGL functions

#define use_shader_program glUseProgram
#define deinit_shader_program glDeleteProgram

////////// These macros pertain to window + rendering defaults

#define WINDOW_W 800
#define WINDOW_H 600

#define OPENGL_MAJOR_VERSION 3
#define OPENGL_MINOR_VERSION 3

#define DEPTH_BUFFER_BITS 24
#define MULTISAMPLE_SAMPLES 4

#define USE_VSYNC
#define ENABLE_ANISOTROPIC_FILTERING
#define USE_GAMMA_CORRECTION
// #define FORCE_SOFTWARE_RENDERER

//////////

typedef struct {
	SDL_Window* const window;
	SDL_GLContext opengl_context;
} Screen;

typedef enum {
	CompileVertexShader,
	CompileFragmentShader,
	LinkShaders
} ShaderCompilationStep;

typedef enum {
	LaunchSDL,
	LaunchGLAD,
	OpenImageFile,
	CreateMesh,
	TextureIDIsTooLarge,
	TextureSetIsTooLarge,
	CreateFramebuffer,
	InitializeList
} FailureType;

typedef struct {
	GLuint shader_program, vertex_array, *vertex_buffers, *textures;
	GLsizei num_vertex_buffers, num_textures;
	void* any_data; // If a demo need to pass in extra info to the drawer, it can do it through here
} StateGL;

//////////

const Uint8* keys;

//////////

inlinable void fail(const GLchar* const msg, const FailureType failure_type) {
	fprintf(stderr, "Could not %s.\n", msg);
	exit((int) failure_type + 1);
}

// Excluded: resize_window_if_needed, set_triangle_fill_mode, query_for_application_exit, fail_on_shader_creation_error

Screen init_screen(const GLchar* const title);
void deinit_screen(const Screen* const screen);

void make_application(void (*const drawer) (const StateGL* const),
	StateGL (*const init) (void), void (*const deinit) (const StateGL* const));

void loop_application(const Screen* const screen, void (*const drawer) (const StateGL* const),
	StateGL (*const init) (void), void (*const deinit) (const StateGL* const));

// Deinitializes shader, unbinds vbos from vao, deletes vbos, textures, and vao
void deinit_demo_vars(const StateGL* const sgl);

//////////

GLuint init_vao(void);
GLuint* init_vbos(const GLsizei num_buffers, ...);
void bind_vbos_to_vao(const GLuint* const vbos, const GLuint num_vbos, ...);

GLuint init_shader_program(const GLchar* const vertex_shader, const GLchar* const fragment_shader);

void enable_all_culling(void);

// Note: `x` and `y` are top-down here (making them technically `x` and `z`).
byte* map_point(byte* const map, const byte x, const byte y, const byte map_width);

const char* get_gl_error(void);

#endif
