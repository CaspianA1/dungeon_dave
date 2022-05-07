#ifndef UTILS_H
#define UTILS_H

#include "buffer_defs.h"

////////// These macros pertain to debugging

#define GL_ERR_CHECK printf("GL error check: %s\n", get_gl_error());
#define SDL_ERR_CHECK printf("SDL error check: '%s'\n", SDL_GetError());

#define KEY_FLY SDL_SCANCODE_1
#define KEY_TOGGLE_WIREFRAME_MODE SDL_SCANCODE_2
#define KEY_PRINT_POSITION SDL_SCANCODE_3
#define KEY_PRINT_OPENGL_ERROR SDL_SCANCODE_4
#define KEY_PRINT_SDL_ERROR SDL_SCANCODE_5
#define KEY_MAKE_SHADOW_MAP_MIPMAP SDL_SCANCODE_6

#define DEBUG(var, format) printf(#var " = %" #format "\n", (var))
#define DEBUG_FLOAT(var) printf(#var " = %ff\n", (double) (var))
#define DEBUG_VEC2(v) printf(#v " = {%ff, %ff}\n", (double) (v)[0], (double) (v)[1])
#define DEBUG_VEC3(v) printf(#v " = {%ff, %ff, %ff}\n", (double) (v)[0], (double) (v)[1], (double) (v)[2])
#define DEBUG_VEC4(v) printf(#v " = {%ff, %ff, %ff, %ff}\n", (double) (v)[0], (double) (v)[1], (double) (v)[2], (double) (v)[3]);

#define DEBUG_BITS(num) do {\
	printf(#num " = ");\
	for (int16_t i = (sizeof(num) << 3) - 1; i >= 0; i--)\
		putchar((((num) >> i) & 1) + '0');\
	putchar('\n');\
} while (0)

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
	} while (0)

#define INIT_SHADER_BRANCH(shader, name, key) INIT_UNIFORM_VALUE(name, (shader), 1i, keys[SDL_SCANCODE_##key])

////////// These are some general-purpose macros

#define CHECK_BITMASK(bits, mask) (!!((bits) & (mask)))

#define ARRAY_LENGTH(array) (sizeof((array)) / sizeof(*(array)))

#define ON_FIRST_CALL(...) do {\
	static bool first_call = true;\
	if (first_call) {\
		__VA_ARGS__\
		first_call = false;\
	}\
} while (0)

////////// These macros are for handy abstractions over OpenGL functions

#define use_shader glUseProgram
#define deinit_shader glDeleteProgram

#define use_vertex_buffer(vertex_buffer) glBindBuffer(GL_ARRAY_BUFFER, (vertex_buffer))
#define deinit_gpu_buffer(gpu_buffer) glDeleteBuffers(1, &(gpu_buffer))

#define use_vertex_spec glBindVertexArray
#define deinit_vertex_spec(vertex_spec) glDeleteVertexArrays(1, &(vertex_spec))

#define INIT_UNIFORM(name, shader) name##_id = glGetUniformLocation((shader), #name)

#define INIT_UNIFORM_VALUE(name, shader, type_prefix, ...)\
	glUniform##type_prefix(glGetUniformLocation((shader), #name), __VA_ARGS__)

#define INIT_UNIFORM_VALUE_FROM_VARIABLE_NAME(name, shader, type_prefix, ...)\
	glUniform##type_prefix(glGetUniformLocation((shader), (name)), __VA_ARGS__)

#define UPDATE_UNIFORM(name, type_prefix, ...) glUniform##type_prefix(name##_id, __VA_ARGS__)

#define WITH_BINARY_RENDER_STATE(state, ...) do {\
	glEnable((state)); __VA_ARGS__ glDisable((state));\
} while (0)

#define WITHOUT_BINARY_RENDER_STATE(state, ...) do {\
	glDisable((state)); __VA_ARGS__ glEnable((state));\
} while (0)

#define WITH_RENDER_STATE(setter, state, inverse_state, ...) do {\
	setter((state)); __VA_ARGS__ setter((inverse_state));\
} while (0)

////////// These macros pertain to window + rendering defaults

#define WINDOW_W 800
#define WINDOW_H 600

#define OPENGL_MAJOR_VERSION 3
#define OPENGL_MINOR_VERSION 3

#define DEPTH_BUFFER_BITS 24
#define MULTISAMPLE_SAMPLES 8

#define USE_VSYNC

#define USE_GAMMA_CORRECTION
#define USE_MULTISAMPLING
#define USE_POLYGON_ANTIALIASING

#define USE_ANISOTROPIC_FILTERING

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
	TextureIDIsTooLarge,
	CreateFramebuffer,
	CreateSkybox,
	CreateBlankSurface
} FailureType;

//////////

extern const Uint8* keys;
const Uint8* keys;

//////////

void fail(const GLchar* const msg, const FailureType failure_type);

// Excluded: resize_window_if_needed, set_triangle_fill_mode, query_for_application_exit, fail_on_shader_creation_error

Screen init_screen(const GLchar* const title);
void deinit_screen(const Screen* const screen);

void make_application(void (*const drawer) (void* const),
	void* (*const init) (void), void (*const deinit) (void* const));

void loop_application(const Screen* const screen, void (*const drawer) (void* const),
	void* (*const init) (void), void (*const deinit) (void* const));

//////////

void define_vertex_spec_index(const bool is_instanced, const bool vertices_are_floats,
	const byte index, const byte num_components, const byte stride, const size_t initial_offset,
	const GLenum typename);

GLuint init_vertex_spec(void);
GLuint init_gpu_buffer(void);
GLuint init_shader(const GLchar* const vertex_shader, const GLchar* const fragment_shader);

void enable_all_culling(void);

// Note: `x` and `y` are top-down here (making them technically `x` and `z`).
byte sample_map_point(const byte* const map, const byte x, const byte y, const byte map_width);

const char* get_gl_error(void);

#endif
