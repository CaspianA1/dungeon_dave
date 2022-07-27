#include "utils/utils.h"
#include "utils/texture.h"
#include "data/constants.h"

typedef struct {
	SDL_Window* const window;
	SDL_GLContext opengl_context;
} Screen;

//////////

static Screen init_screen(const GLchar* const title, const byte opengl_major_minor_version[2],
	const byte depth_buffer_bits, const byte multisample_samples, const GLint window_size[2]) {

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) FAIL(LoadSDL, "SDL loading failed: '%s'", SDL_GetError());

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, opengl_major_minor_version[0]);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, opengl_major_minor_version[1]);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, depth_buffer_bits);

	#ifdef USE_GAMMA_CORRECTION
	SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);
	#endif

	#ifdef USE_MULTISAMPLING
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, multisample_samples);
	#endif

	#ifdef FORCE_SOFTWARE_RENDERER
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 0);
	#endif

	const GLint window_w = window_size[0], window_h = window_size[1];

	Screen screen = {
		.window = SDL_CreateWindow(title,
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			window_w, window_h, SDL_WINDOW_OPENGL)
	};

	if (screen.window == NULL) FAIL(LoadSDL, "Window creation failed: '%s'", SDL_GetError());

	screen.opengl_context = SDL_GL_CreateContext(screen.window);
	if (screen.opengl_context == NULL) FAIL(LoadOpenGL, "Could not load an OpenGL context: '%s'", SDL_GetError());
	SDL_GL_MakeCurrent(screen.window, screen.opengl_context);

	SDL_SetRelativeMouseMode(SDL_TRUE);
	SDL_WarpMouseInWindow(screen.window, window_w >> 1, window_h >> 1);

	SDL_GL_SetSwapInterval(
		#ifdef USE_VSYNC
		1
		#else
		0
		#endif
	);

	if (!gladLoadGL()) FAIL(LoadOpenGL, "%s", "GLAD could not load for some reason");

	#ifdef USE_GAMMA_CORRECTION
	glEnable(GL_FRAMEBUFFER_SRGB);
	#endif

	#ifdef USE_MULTISAMPLING
	glEnable(GL_MULTISAMPLE);
	#endif

	#ifdef USE_POLYGON_ANTIALIASING
	glEnable(GL_POLYGON_SMOOTH);
	glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
	#endif

	return screen;
}

static void deinit_screen(const Screen* const screen) {
	SDL_GL_DeleteContext(screen -> opengl_context);
	SDL_DestroyWindow(screen -> window);
	SDL_Quit();
}

//////////

static void resize_window_if_needed(SDL_Window* const window, const Uint8* const keys) {
	static bool window_resized_last_tick = false, window_is_fullscreen = false;
	static GLint desktop_width, desktop_height;

	ON_FIRST_CALL( // TODO: make this a runtime constant
		SDL_DisplayMode display_mode;
		SDL_GetDesktopDisplayMode(0, &display_mode);
		desktop_width = display_mode.w;
		desktop_height = display_mode.h;
	);

	const bool resize_attempt = keys[constants.keys.toggle_fullscreen_window];

	if (!window_resized_last_tick && resize_attempt) {
		window_is_fullscreen = !window_is_fullscreen;
		window_resized_last_tick = true;

		/* There's a branch here because the order in which the resolution
		and window mode is changed matters. If setting the window size and
		the fullscreen mode is done in the wrong order, a half-framebuffer
		may be shown on a window, which will look odd. */
		if (window_is_fullscreen) {
			SDL_SetWindowSize(window, desktop_width, desktop_height);
			SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
			glViewport(0, 0, desktop_width, desktop_height);
		}
		else {
			const GLint window_w = constants.window.size[0], window_h = constants.window.size[1];

			SDL_SetWindowFullscreen(window, 0);
			SDL_SetWindowSize(window, window_w, window_h);
			SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
			glViewport(0, 0, window_w, window_h);
		}

	}
	else if (!resize_attempt) window_resized_last_tick = false;
}

static void set_triangle_fill_mode(const Uint8* const keys) {
	static bool in_triangle_fill_mode = true, changed_mode_last_tick = false;

	if (keys[KEY_TOGGLE_WIREFRAME_MODE]) {
		if (!changed_mode_last_tick) {
			in_triangle_fill_mode = !in_triangle_fill_mode;
			changed_mode_last_tick = true;
			glPolygonMode(GL_FRONT_AND_BACK, in_triangle_fill_mode ? GL_FILL : GL_LINE);
			glEnable(GL_LINE_SMOOTH);
		}
	}
	else changed_mode_last_tick = false;
}

static bool application_should_exit(const Uint8* const keys) {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT) return true;
	}

	/* On Ubuntu, the only way to activate the SDL_QUIT event type
	is by pressing the window exit button (not through pressing ctrl-w or ctrl-q!).
	That isn't possible for this application, since the mouse is locked in the window.
	Since exiting doesn't work normally, a ctrl key followed by an exit activation key
	serves as a manual workaround to this problem. */

	const bool // On Ubuntu, SDL_QUIT is not caught by SDL_PollEvent, so this circumvents that
		ctrl_key = keys[constants.keys.ctrl[0]] || keys[constants.keys.ctrl[1]],
		activate_exit_key = keys[constants.keys.activate_exit[0]] || keys[constants.keys.activate_exit[1]];

	return ctrl_key && activate_exit_key;
}

//////////

static void loop_application(const Screen* const screen, void* const app_context, void (*const drawer) (void* const, const Event* const)) {
	SDL_Window* const window = screen -> window;
	const Uint8* const keys = SDL_GetKeyboardState(NULL);

	////////// Timing-related variables

	#ifndef USE_VSYNC
	const GLfloat max_delay = constants.milliseconds_per_second / get_runtime_constant(RefreshRate);
	#endif

	const GLfloat one_over_time_frequency = 1.0f / SDL_GetPerformanceFrequency();

	GLfloat secs_elapsed_between_frames = 0.0f;
	Uint64 time_counter_for_last_frame = SDL_GetPerformanceCounter();

	//////////

	while (!application_should_exit(keys)) {
		////////// Getting `time_before_tick_ms`, resizing the window, and setting the triangle fill mode

		const Uint32 time_before_tick_ms = SDL_GetTicks();

		resize_window_if_needed(window, keys);
		set_triangle_fill_mode(keys);

		////////// Getting the next event, drawing the screen, debugging errors, and swapping the framebuffer

		const Event event = get_next_event(time_before_tick_ms, secs_elapsed_between_frames, keys);

		drawer(app_context, &event);

		if (keys[KEY_PRINT_OPENGL_ERROR]) GL_ERR_CHECK;
		if (keys[KEY_PRINT_SDL_ERROR]) SDL_ERR_CHECK;

		SDL_GL_SwapWindow(window);

		////////// Updating `secs_elapsed_between_frames` and `time_counter_for_last_frame`, and delaying if needed

		const Uint64 time_counter_for_curr_frame = SDL_GetPerformanceCounter();
		const Uint64 time_counter_delta = time_counter_for_curr_frame - time_counter_for_last_frame;

		secs_elapsed_between_frames = (GLfloat) time_counter_delta * one_over_time_frequency;
		time_counter_for_last_frame = time_counter_for_curr_frame;

		#ifndef USE_VSYNC
		const GLfloat wait_for_exact_fps = max_delay - secs_elapsed_between_frames * constants.milliseconds_per_second;
		if (wait_for_exact_fps > 0.0f) SDL_Delay((Uint32) wait_for_exact_fps);
		#endif
	}
}

void make_application(void (*const drawer) (void* const, const Event* const),
	void* (*const init) (void), void (*const deinit) (void* const)) {

	const Screen screen = init_screen(constants.window.app_name,
		constants.window.opengl_major_minor_version, constants.window.depth_buffer_bits,
		constants.window.multisample_samples, constants.window.size);

	printf("---\nvendor = %s\nrenderer = %s\nversion = %s\n---\n",
		glGetString(GL_VENDOR), glGetString(GL_RENDERER), glGetString(GL_VERSION));

	void* const app_context = init();
	loop_application(&screen, app_context, drawer);
	deinit(app_context);
	deinit_screen(&screen);
}

// `x` and `y` are top-down
byte sample_map_point(const byte* const map, const byte x, const byte y, const byte map_width) {
	return map[y * map_width + x];
}

const GLchar* get_GL_error(void) {
	#define ERROR_CASE(error) case GL_##error: return #error;

	switch (glGetError()) {
		ERROR_CASE(NO_ERROR);
		ERROR_CASE(INVALID_ENUM);
		ERROR_CASE(INVALID_VALUE);
		ERROR_CASE(INVALID_OPERATION);
		ERROR_CASE(INVALID_FRAMEBUFFER_OPERATION);
		ERROR_CASE(OUT_OF_MEMORY);
		default: return "Unknown error";
	}

	#undef ERROR_CASE
}

FILE* open_file_safely(const GLchar* const path, const GLchar* const mode) {
	FILE* const file = fopen(path, mode);
	if (file == NULL) FAIL(OpenFile, "could not open a file with the path of '%s'", path);
	return file;
}
