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
#define SCR_W 800
#define SCR_H 600
#define FPS 60
#define FOV 90.0f

#define KEY_PRINT_POSITION SDL_SCANCODE_1
#define KEY_PRINT_OPENGL_ERROR SDL_SCANCODE_2

#define SDL_PIXEL_FORMAT SDL_PIXELFORMAT_BGRA32
#define OPENGL_INPUT_PIXEL_FORMAT GL_BGRA
#define OPENGL_INTERNAL_PIXEL_FORMAT GL_RGBA

#define OPENGL_COLOR_CHANNEL_TYPE GL_UNSIGNED_BYTE

#define OPENGL_TEX_MAG_FILTER GL_LINEAR
#define OPENGL_TEX_MIN_FILTER GL_LINEAR_MIPMAP_LINEAR
// Mip level should not change per skybox, so no trilinear needed
#define OPENGL_SKYBOX_TEX_MIN_FILTER GL_LINEAR_MIPMAP_NEAREST
#define ENABLE_ANISOTROPIC_FILTERING

// GL_MIRRORED_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER
enum {tex_repeating = GL_REPEAT, tex_nonrepeating = GL_CLAMP_TO_EDGE};

const Uint8* keys;

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
	OpenImageFile,
	MeshOutOfBounds
} FailureType;

typedef struct {
	GLuint shader_program, vertex_array, *vertex_buffers, *textures;
	int num_vertex_buffers, num_textures;
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
	StateGL (*const init)(void), void (*const deinit)(const StateGL* const), const byte fps);

// Deinitializes shader, unbinds vbos from vao, deletes vbos, textures, and vao
void deinit_demo_vars(const StateGL* const sgl);

//////////

GLuint init_vao(void);
GLuint* init_vbos(const int num_buffers, ...);
void bind_vbos_to_vao(const GLuint* const vbos, const int num_vbos, ...);

GLuint init_shader_program(const char* const vertex_shader, const char* const fragment_shader);

SDL_Surface* init_surface(const char* const path);
void deinit_surface(SDL_Surface* const surface);

GLuint* init_textures(const int num_textures, ...);
void select_texture_for_use(const GLuint texture, const GLuint shader_program);

void enable_all_culling(void);
void draw_triangles(const int num_triangles);

static inline GLfloat to_radians(const GLfloat degrees);
