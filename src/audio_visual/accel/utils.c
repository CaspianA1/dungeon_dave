#include <SDL2/SDL.h>
#include <GL/glew.h>

typedef struct {
	SDL_Window* window;
	SDL_GLContext opengl_context;
} Screen;

typedef uint_fast8_t byte;

#define OPENGL_MAJOR_VERSION 3
#define OPENGL_MINOR_VERSION 1
#define SCR_W 800
#define SCR_H 600

inline void fail(const char* const msg, const int exit_code) {
	fprintf(stderr, "Could not %s; SDL error = %s, OpenGL error = %s\n", msg,
		SDL_GetError(), glewGetErrorString(glGetError()));
	exit(exit_code);
}

Screen init_screen(const char* const title) {
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) fail("launch SDL", 1);

	SDL_SetHintWithPriority(SDL_HINT_RENDER_VSYNC, "1", SDL_HINT_OVERRIDE);
	SDL_SetHintWithPriority(SDL_HINT_RENDER_BATCHING, "1", SDL_HINT_OVERRIDE);
	SDL_SetHintWithPriority(SDL_HINT_RENDER_DRIVER, "opengl", SDL_HINT_OVERRIDE);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, OPENGL_MAJOR_VERSION);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, OPENGL_MINOR_VERSION);
	// set bufs

	Screen screen;

	screen.window = SDL_CreateWindow(title,
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		SCR_W, SCR_H, SDL_WINDOW_OPENGL);

	screen.opengl_context = SDL_GL_CreateContext(screen.window);

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) fail("initialize glew", 2);

	return screen;
}

void deinit_screen(const Screen* const screen) {
	SDL_GL_DeleteContext(screen -> opengl_context);
	SDL_DestroyWindow(screen -> window);
	SDL_Quit();
}

void loop_application(const Screen* const screen, byte (*loop_fn)(const Screen* const), const byte fps) {
	const double max_delay = 1000.0 / fps;
	byte running = 1;

	while (running) {
		const Uint32 before = SDL_GetTicks();
		if (!loop_fn(screen)) running = 0;
		const int wait = max_delay - (SDL_GetTicks() - before);
		if (wait > 0) SDL_Delay(wait);
	}
}

byte loop_tick(const Screen* const screen) {
	SDL_Event event;

	byte quitting = 0;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT) quitting = 1;
	}

	(void) screen;

	return !quitting;
}

int main(void) {
	const Screen screen = init_screen("Accel Demo");
	loop_application(&screen, loop_tick, 60);
	deinit_screen(&screen);
}
