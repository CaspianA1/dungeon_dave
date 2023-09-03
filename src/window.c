#include "window.h"
#include "utils/failure.h" // For `FAIL`
#include "utils/texture.h" // For `global_anisotropic_filtering_level`
#include "data/constants.h" // For various constants
#include "utils/macro_utils.h" // For `ON_FIRST_CALL`

typedef struct {
	SDL_Window* const window;
	SDL_GLContext opengl_context;
} Screen;

//////////

static Screen init_screen(const WindowConfig* const config) {
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
		FAIL(LoadSDL, "SDL loading failed: '%s'", SDL_GetError());

	const byte* const opengl_major_minor_version = config -> opengl_major_minor_version;

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, opengl_major_minor_version[0]);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, opengl_major_minor_version[1]);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, config -> depth_buffer_bits);
	SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);

	if (config -> enabled.multisampling) {
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, config -> multisample_samples);
	}

	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, !config -> enabled.software_renderer);

	const uint16_t* const window_size = config -> window_size;
	const uint16_t window_w = window_size[0], window_h = window_size[1];

	// TODO: force high-DPI if it's available

	Screen screen = {
		.window = SDL_CreateWindow(config -> app_name,
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			window_w, window_h, SDL_WINDOW_OPENGL)
	};

	if (screen.window == NULL) FAIL(LoadSDL, "Window creation failed: '%s'", SDL_GetError());

	screen.opengl_context = SDL_GL_CreateContext(screen.window);
	if (screen.opengl_context == NULL) FAIL(LoadOpenGL, "Could not load an OpenGL context: '%s'", SDL_GetError());

	SDL_GL_MakeCurrent(screen.window, screen.opengl_context);
	SDL_GL_SetSwapInterval(config -> enabled.vsync ? 1 : 0);

	//////////

	if (!gladLoadGL()) FAIL(LoadOpenGL, "%s", "GLAD could not load for some reason");

	// Disabling anisotropic filtering globally
	if (!config -> enabled.aniso_filtering)
		GLAD_GL_EXT_texture_filter_anisotropic = 0;
	else {
		GLfloat max_anisotropic; // Otherwise, setting the global anisotropic filtering level
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_anisotropic);
		global_anisotropic_filtering_level = fminf(max_anisotropic, config -> aniso_filtering_level);
	}

	//////////

	glEnable(GL_FRAMEBUFFER_SRGB);
	if (config -> enabled.multisampling) glEnable(GL_MULTISAMPLE);

	return screen;
}

static void deinit_screen(const Screen* const screen) {
	SDL_GL_DeleteContext(screen -> opengl_context);
	SDL_DestroyWindow(screen -> window);
	SDL_Quit();
}

//////////

static void resize_window_if_needed(SDL_Window* const window, const WindowConfig* const config, const Uint8* const keys) {
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
			const uint16_t* const window_size = config -> window_size;
			const uint16_t window_w = window_size[0], window_h = window_size[1];

			SDL_SetWindowFullscreen(window, 0);
			SDL_SetWindowSize(window, window_w, window_h);
			SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
			glViewport(0, 0, window_w, window_h);
		}

	}
	else if (!resize_attempt) window_resized_last_tick = false;
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
	serves as a manual workaround to this problem. TODO: document this in the README. */

	const bool // On Ubuntu, `SDL_QUIT` is not caught by `SDL_PollEvent`, so this circumvents that
		ctrl_key = keys[constants.keys.ctrl[0]] || keys[constants.keys.ctrl[1]],
		activate_exit_key = keys[constants.keys.activate_exit[0]] || keys[constants.keys.activate_exit[1]];

	return ctrl_key && activate_exit_key;
}

//////////

static void loop_application(
	const Screen* const screen, const WindowConfig* const config,
	void* const app_context, const drawer_t drawer) {

	SDL_Window* const window = screen -> window;
	const Uint8* const keys = SDL_GetKeyboardState(NULL);

	////////// Finding the refresh rate

	SDL_DisplayMode display_mode;
	SDL_GetCurrentDisplayMode(0, &display_mode);

	const bool vsync_is_enabled = config -> enabled.vsync;

	// If the display mode refresh rate is 0, it is considered not available
	const byte refresh_rate = (display_mode.refresh_rate == 0 || !vsync_is_enabled)
		? config -> default_fps : (byte) display_mode.refresh_rate;

	////////// Timing-related variables

	const GLfloat max_delay = constants.milliseconds_per_second / (GLfloat) refresh_rate;
	const GLfloat one_over_time_frequency = 1.0f / SDL_GetPerformanceFrequency();

	GLfloat secs_elapsed_between_frames = 0.0f;
	Uint64 time_counter_for_last_frame = SDL_GetPerformanceCounter();
	bool mouse_is_currently_visible = true;

	//////////

	while (!application_should_exit(keys)) {
		////////// Getting `time_before_tick_ms`, resizing the window, and setting the triangle fill mode

		const Uint32 time_before_tick_ms = SDL_GetTicks();
		resize_window_if_needed(window, config, keys);

		////////// Getting the next event, drawing the screen, and swapping the framebuffer

		const Event event = get_next_event(time_before_tick_ms, secs_elapsed_between_frames, keys);
		const bool mouse_should_be_visible = drawer(app_context, &event, config);

		if (mouse_should_be_visible != mouse_is_currently_visible) {
			mouse_is_currently_visible = mouse_should_be_visible;
			// TODO: fix the 'unsupported' error arising from this on the second-made level on Fedora (does this happen on MacOS?)
			SDL_SetRelativeMouseMode(mouse_should_be_visible ? SDL_FALSE : SDL_TRUE);
		}

		SDL_GL_SwapWindow(window);

		////////// Updating `secs_elapsed_between_frames` and `time_counter_for_last_frame`, and delaying if needed

		const Uint64 time_counter_for_curr_frame = SDL_GetPerformanceCounter();
		const Uint64 time_counter_delta = time_counter_for_curr_frame - time_counter_for_last_frame;

		secs_elapsed_between_frames = (GLfloat) time_counter_delta * one_over_time_frequency;
		time_counter_for_last_frame = time_counter_for_curr_frame;

		if (!vsync_is_enabled) {
			const GLfloat wait_for_exact_fps = max_delay - secs_elapsed_between_frames * constants.milliseconds_per_second;
			if (wait_for_exact_fps > 0.0f) SDL_Delay((Uint32) wait_for_exact_fps);
		}
	}
}

void make_application(
	const WindowConfig* const config,
	void* (*const init) (const WindowConfig* const),
	void (*const deinit) (void* const),
	const drawer_t drawer) {

	const Screen screen = init_screen(config);
	void* const app_context = init(config);

	loop_application(&screen, config, app_context, drawer);
	deinit(app_context);
	deinit_screen(&screen);
}
