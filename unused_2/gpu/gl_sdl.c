#include <SDL2/SDL.h>
// #include <GL/glew.h>
#include <GL/gl.h>

enum {w = 800, h = 600};

#define FAIL(msg) {fprintf(stderr, "Could not " msg "\n"); return 1;}
#define DEBUG(var, format) printf(#var " = %" #format "\n", var)

// https://www.khronos.org/opengl/wiki/Tutorial1:_Creating_a_Cross_Platform_OpenGL_3.2_Context_in_SDL_(C_/_SDL)
// clang -O3 -lSDL2 -framework OpenGL gl_sdl.c && ./a.out

int main(void) {
	if (SDL_Init(SDL_INIT_VIDEO) < 0) FAIL("initialize SDL");

	int a, b, c, d, e, f, g;

	a = SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	b = SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	c = SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	d = SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

	// Turn on double buffering with a 24bit Z buffer. You may need to change this to 16 or 32 for your system
	e = SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	f = SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	SDL_Window* const window = SDL_CreateWindow("gl_sdl", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, SDL_WINDOW_OPENGL);
	if (window == NULL) FAIL("create a window");

	SDL_GLContext context = SDL_GL_CreateContext(window);
	if (context == NULL) FAIL("create a context");

	/*
	glewExperimental = GL_TRUE;
	GLenum maybe_error = glewInit();
	if (maybe_error != GLEW_OK) FAIL("initialize GLEW");
	*/

	// This makes our buffer swap syncronized with the monitor's vertical refresh
	g = SDL_GL_SetSwapInterval(1);

	// Checking for any possible errors from SDL_GL calls
	printf("a = %d, b = %d, c = %d, d = %d, e = %d, f = %d, g = %d\n", a, b, c, d, e, f, g);
	printf("vendor = %s\nrenderer = %s\nversion = %s\n", glGetString(GL_VENDOR), glGetString(GL_RENDERER), glGetString(GL_VERSION));

	SDL_Event event;
	while (1) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				SDL_GL_DeleteContext(context);
				SDL_DestroyWindow(window);
				SDL_Quit();
				return 0;
			}
		}

		// Blue
		glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		SDL_GL_SwapWindow(window);
		SDL_Delay(20);
	}
}
