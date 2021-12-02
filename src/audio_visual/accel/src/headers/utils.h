#ifndef UTILS_H
#define UTILS_H

#include <SDL2/SDL.h>
#include <GL/glew.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#include <cglm/cglm.h>
#pragma GCC diagnostic pop

#define DEBUG(var, format) printf(#var " = %" #format "\n", var)
#define GL_ERR_CHECK printf("GL error check: '%s'\n", glewGetErrorString(glGetError()))
#define OPENGL_MAJOR_VERSION 3
#define OPENGL_MINOR_VERSION 3

// 800 by 600
#define SCR_W 1440
#define SCR_H 900
#define FOV 90.0f

#define KEY_PRINT_POSITION SDL_SCANCODE_1
#define KEY_PRINT_OPENGL_ERROR SDL_SCANCODE_2

#define DEPTH_BUFFER_BITS 24
#define MULTISAMPLE_SAMPLES 8

const Uint8* keys;

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
	MeshOutOfBounds,
	TextureSetMismatchingImageSize
} FailureType;

typedef struct {
	GLuint shader_program, vertex_array, *vertex_buffers, *textures;
	GLsizei num_vertex_buffers, num_textures;
	void* any_data; // If a demo need to pass in extra info to the drawer, it can do it through here
} StateGL;

extern inline void fail(const char* const msg, const FailureType failure_type) {
	fprintf(stderr, "Could not %s; SDL error = '%s', OpenGL error = '%s'\n", msg,
		SDL_GetError(), glewGetErrorString(glGetError()));
	exit(failure_type + 1);
}

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

static inline GLfloat to_radians(const GLfloat degrees);

#endif
