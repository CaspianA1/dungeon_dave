#include <SDL2/SDL.h>
#include <GL/glew.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#include <cglm/cglm.h>
#pragma GCC diagnostic pop

// #define DEMO_1
// #define DEMO_2
// #define DEMO_3
#define DEMO_4

#define DEBUG(var, format) printf(#var " = %" #format "\n", var)
#define GL_ERR_CHECK printf("GL error check: '%s'\n", glewGetErrorString(glGetError()))
#define OPENGL_MAJOR_VERSION 3
#define OPENGL_MINOR_VERSION 3
#define SCR_W 800
#define SCR_H 600

#define SDL_PIXEL_FORMAT SDL_PIXELFORMAT_RGBA5551
#define OPENGL_PIXEL_FORMAT GL_RGBA // internal format is same as input format from SDL
#define OPENGL_COLOR_CHANNEL_TYPE GL_UNSIGNED_BYTE
#define OPENGL_TEX_MAG_FILTER GL_NEAREST
#define OPENGL_TEX_MIN_FILTER GL_LINEAR_MIPMAP_LINEAR

typedef uint_fast8_t byte;

typedef struct {
	SDL_Window* const window;
	SDL_GLContext opengl_context;
} Screen;

typedef enum {
	LaunchSDL,
	LaunchGLEW,
	CompileShader,
	LinkShaders,
	OpenImageFile
} FailureType;

typedef struct {
	GLuint shader_program;
	GLuint vertex_array;
	GLuint* vertex_buffers;
	int num_vertex_buffers;
} StateGL;

inline void fail(const char* const msg, const FailureType failure_type) {
	fprintf(stderr, "Could not %s; SDL error = '%s', OpenGL error = '%s'\n", msg,
		SDL_GetError(), glewGetErrorString(glGetError()));
	exit(failure_type + 1);
}

Screen init_screen(const char* const title);
void deinit_screen(const Screen* const screen);

void make_application(void (*const drawer)(const StateGL),
	StateGL (*const init)(void), void (*const deinit)(StateGL));

void loop_application(const Screen* const screen, void (*const drawer)(const StateGL),
	StateGL (*const init)(void), void (*const deinit)(StateGL), const byte fps);

GLuint init_shader_program(const char* const vertex_shader, const char* const fragment_shader);
void bind_vbos_to_vao(const GLuint* const vbos, const int num_vbos);
void unbind_vbos_from_vao(const int num_vbos);
GLuint init_vao(void);
GLuint* init_vbos(const int num_buffers, ...);
GLuint init_texture(const char* const path);

void deinit_demo_vars(const StateGL sgl); // Deletes shader program, vbos, and vao
