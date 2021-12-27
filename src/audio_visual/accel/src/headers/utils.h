#ifndef UTILS_H
#define UTILS_H

#include <SDL2/SDL.h>
#include <GL/glew.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#include <cglm/cglm.h>
#pragma GCC diagnostic pop

#define DEBUG(var, format) printf(#var " = %" #format "\n", var)
#define DEBUG_VEC(v) printf(#v " = {%lf, %lf, %lf}\n", (double) v[0], (double) v[1], (double) v[2])
#define GL_ERR_CHECK printf("GL error check: '%s'\n", glewGetErrorString(glGetError()))
#define SDL_ERR_CHECK printf("SDL error check: '%s'\n", SDL_GetError());
#define OPENGL_MAJOR_VERSION 3
#define OPENGL_MINOR_VERSION 3
// #define FORCE_SOFTWARE_RENDERER

#define WINDOW_W 800
#define WINDOW_H 600

#define KEY_TOGGLE_FULLSCREEN_WINDOW SDL_SCANCODE_ESCAPE
#define KEY_PRINT_POSITION SDL_SCANCODE_1
#define KEY_PRINT_OPENGL_ERROR SDL_SCANCODE_2
#define KEY_PRINT_SDL_ERROR SDL_SCANCODE_3

#define DEPTH_BUFFER_BITS 24
#define MULTISAMPLE_SAMPLES 8

typedef uint_fast8_t byte;

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
	LaunchGLEW,
	OpenImageFile,
	MeshOutOfBounds, // TODO: remove
	TextureIDIsTooLarge,
	TextureSetIsTooLarge
} FailureType;

typedef struct {
	GLuint shader_program, vertex_array, *vertex_buffers, *textures;
	GLsizei num_vertex_buffers, num_textures;
	void* any_data; // If a demo need to pass in extra info to the drawer, it can do it through here
} StateGL;

const Uint8* keys;

extern inline void fail(const char* const msg, const FailureType failure_type) {
	fprintf(stderr, "Could not %s; SDL error = '%s', OpenGL error = '%s'\n", msg,
		SDL_GetError(), glewGetErrorString(glGetError()));
	exit(failure_type + 1);
}

// Excluded: resize_window_if_needed, fail_on_shader_creation_error

Screen init_screen(const char* const title);
void deinit_screen(const Screen* const screen);

void make_application(void (*const drawer)(const StateGL* const),
	StateGL (*const init)(void), void (*const deinit)(const StateGL* const));
void loop_application(const Screen* const screen, void (*const drawer)(const StateGL* const),
	StateGL (*const init)(void), void (*const deinit)(const StateGL* const));

// Deinitializes shader, unbinds vbos from vao, deletes vbos, textures, and vao
void deinit_demo_vars(const StateGL* const sgl);

//////////

GLuint init_vao(void);
GLuint* init_vbos(const GLsizei num_buffers, ...);
void bind_vbos_to_vao(const GLuint* const vbos, const GLsizei num_vbos, ...);

GLuint init_shader_program(const char* const vertex_shader, const char* const fragment_shader);

void enable_all_culling(void);
void draw_triangles(const GLsizei num_triangles);

#endif
