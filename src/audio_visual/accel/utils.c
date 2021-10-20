#include <SDL2/SDL.h>
#include <GL/glew.h>

#define DEBUG(var, format) printf(#var " = %" #format "\n", var)
#define GL_ERR_CHECK printf("GL error check: '%s'\n", glewGetErrorString(glGetError()))
#define OPENGL_MAJOR_VERSION 3
#define OPENGL_MINOR_VERSION 3
#define SCR_W 800
#define SCR_H 600

typedef uint_fast8_t byte;

typedef struct {
	SDL_Window* const window;
	SDL_GLContext opengl_context;
} Screen;

typedef enum {
	LaunchSDL,
	LaunchGLEW,
	CompileShader,
	LinkShaders
} FailureType;

typedef struct {
	GLenum shader_program;
	GLuint vertex_array, vertex_buffer;
} DemoVars;

inline void fail(const char* const msg, const FailureType failure_type) {
	fprintf(stderr, "Could not %s; SDL error = '%s', OpenGL error = '%s'\n", msg,
		SDL_GetError(), glewGetErrorString(glGetError()));
	exit(failure_type + 1);
}

Screen init_screen(const char* const title) {
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) fail("launch SDL", LaunchSDL);

	SDL_SetHintWithPriority(SDL_HINT_RENDER_VSYNC, "1", SDL_HINT_OVERRIDE);
	SDL_SetHintWithPriority(SDL_HINT_RENDER_BATCHING, "1", SDL_HINT_OVERRIDE);
	SDL_SetHintWithPriority(SDL_HINT_RENDER_DRIVER, "opengl", SDL_HINT_OVERRIDE);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, OPENGL_MAJOR_VERSION);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, OPENGL_MINOR_VERSION);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	Screen screen = {
		.window = SDL_CreateWindow(title,
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			SCR_W, SCR_H, SDL_WINDOW_OPENGL)
	};

	screen.opengl_context = SDL_GL_CreateContext(screen.window);

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) fail("initialize glew", LaunchGLEW);

	return screen;
}

void deinit_screen(const Screen* const screen) {
	SDL_GL_DeleteContext(screen -> opengl_context);
	SDL_DestroyWindow(screen -> window);
	SDL_Quit();
}

void loop_application(const Screen* const screen, void (*const drawer)(const DemoVars),
	DemoVars (*const init)(void), void (*const deinit)(DemoVars), const byte fps) {

	const double max_delay = 1000.0 / fps;
	byte running = 1;
	SDL_Event event;

	const DemoVars dv = init();

	while (running) {
		const Uint32 before = SDL_GetTicks();

		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) running = 0;
		}

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		drawer(dv);
		SDL_GL_SwapWindow(screen -> window);

		const int wait = max_delay - (SDL_GetTicks() - before);
		if (wait > 0) SDL_Delay(wait);
	}

	deinit(dv);
}

void make_application(void (*const drawer)(const DemoVars),
	DemoVars (*const init)(void), void (*const deinit)(DemoVars)) {

	const Screen screen = init_screen("Accel Demo");

	printf("vendor = %s\nrenderer = %s\nversion = %s\n---\n",
		glGetString(GL_VENDOR), glGetString(GL_RENDERER), glGetString(GL_VERSION));

	loop_application(&screen, drawer, init, deinit, 60);
	deinit_screen(&screen);
}

GLenum init_shader_program(const char* const vertex_shader, const char* const fragment_shader) {
	typedef enum {Vertex, Fragment} ShaderType;

	const char* const shaders[2] = {vertex_shader, fragment_shader};
	const GLenum gl_shader_types[2] = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
	GLuint shader_ids[2], program_id = glCreateProgram();

	for (ShaderType type = 0; type < 2; type++) {
		shader_ids[type] = glCreateShader(gl_shader_types[type]);

		const GLenum shader_id = shader_ids[type];
		glShaderSource(shader_id, 1, shaders + type, NULL);
		glCompileShader(shader_id);

		int info_log_length;
		glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &info_log_length);

		if (info_log_length > 0) {
			char* const error_msg = malloc(info_log_length + 1);
			glGetShaderInfoLog(shader_id, info_log_length, NULL, error_msg);
			printf("GLSL compilation error for shader #%d:\n%s\n---\n", type + 1, error_msg);
			free(error_msg);
			fail("compile shader", CompileShader);
		}

		glAttachShader(program_id, shader_id);
	}

	glLinkProgram(program_id);
	int info_log_length;
	glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &info_log_length);
	if (info_log_length > 0) fail("link shaders", LinkShaders);

	for (ShaderType type = 0; type < 2; type++) {
		const GLenum shader_id = shader_ids[type];
		glDetachShader(program_id, shader_id);
		glDeleteShader(shader_id);
	}

	return program_id;
}
