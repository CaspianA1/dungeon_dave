#ifndef UTILS_C
#define UTILS_C

#include "headers/utils.h"
#include "headers/texture.h"
#include "headers/constants.h"

void fail(const GLchar* const msg, const FailureType failure_type) {
	fprintf(stderr, "Could not %s.\n", msg);
	exit((int) failure_type + 1);
}

Screen init_screen(const GLchar* const title) {
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) fail("launch SDL", LaunchSDL);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, OPENGL_MAJOR_VERSION);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, OPENGL_MINOR_VERSION);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, DEPTH_BUFFER_BITS);

	#ifdef USE_GAMMA_CORRECTION
	SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);
	#endif

	#ifdef USE_MULTISAMPLING
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, MULTISAMPLE_SAMPLES);
	#endif

	#ifdef FORCE_SOFTWARE_RENDERER
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 0);
	#endif

	Screen screen = {
		.window = SDL_CreateWindow(title,
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			WINDOW_W, WINDOW_H, SDL_WINDOW_OPENGL)
	};

	if (screen.window == NULL) fail("launch SDL", LaunchSDL);

	screen.opengl_context = SDL_GL_CreateContext(screen.window);
	SDL_GL_MakeCurrent(screen.window, screen.opengl_context);

	SDL_SetRelativeMouseMode(SDL_TRUE);
	SDL_WarpMouseInWindow(screen.window, WINDOW_W >> 1, WINDOW_H >> 1);

	SDL_GL_SetSwapInterval(
		#ifdef USE_VSYNC
		1
		#else
		0
		#endif
	);

	if (!gladLoadGL()) fail("launch GLAD", LaunchGLAD);

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

	keys = SDL_GetKeyboardState(NULL);

	return screen;
}

void deinit_screen(const Screen* const screen) {
	SDL_GL_DeleteContext(screen -> opengl_context);
	SDL_DestroyWindow(screen -> window);
	SDL_Quit();
}

void make_application(void (*const drawer)(void* const),
	void* (*const init) (void), void (*const deinit) (void* const)) {

	const Screen screen = init_screen(constants.app_name);

	printf("vendor = %s\nrenderer = %s\nversion = %s\n---\n",
		glGetString(GL_VENDOR), glGetString(GL_RENDERER), glGetString(GL_VERSION));

	loop_application(&screen, drawer, init, deinit);
	deinit_screen(&screen);
}

static void resize_window_if_needed(SDL_Window* const window) {
	static bool window_resized_last_tick = false, window_is_fullscreen = false;
	static GLint desktop_width, desktop_height;

	ON_FIRST_CALL(  // TODO: make this a runtime constant
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
			SDL_SetWindowFullscreen(window, 0);
			SDL_SetWindowSize(window, WINDOW_W, WINDOW_H);
			SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
			glViewport(0, 0, WINDOW_W, WINDOW_H);
		}

	}
	else if (!resize_attempt) window_resized_last_tick = false;
}

static void set_triangle_fill_mode(void) {
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

static bool application_should_exit(void) {
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

void loop_application(const Screen* const screen, void (*const drawer) (void* const),
	void* (*const init) (void), void (*const deinit) (void* const)) {

	#ifndef USE_VSYNC
	const GLfloat one_over_performance_freq = 1.0f / SDL_GetPerformanceFrequency();
	#endif

	void* const app_context = init();
	SDL_Window* const window = screen -> window;

	while (!application_should_exit()) {
		#ifndef USE_VSYNC
		const Uint64 before = SDL_GetPerformanceCounter();
		#endif

		resize_window_if_needed(screen -> window);
		set_triangle_fill_mode();

		drawer(app_context);

		if (keys[KEY_PRINT_OPENGL_ERROR]) GL_ERR_CHECK;
		if (keys[KEY_PRINT_SDL_ERROR]) SDL_ERR_CHECK;

		SDL_GL_SwapWindow(window);

		#ifndef USE_VSYNC
		// The refresh rate may change, so it is re-fetched
		const GLfloat max_delay = 1000.0f / get_runtime_constant(RefreshRate);
		const GLfloat ms_elapsed = (GLfloat) (SDL_GetPerformanceCounter() - before) * one_over_performance_freq * 1000.0f;
		const GLfloat wait_for_exact_fps = max_delay - ms_elapsed;
		if (wait_for_exact_fps > 12.0f) SDL_Delay((Uint32) (wait_for_exact_fps - 0.5f)); // SDL_Delay tends to be late, so 0.5f accounts for that
		#endif
	}

	deinit(app_context);
}

GLuint init_vao(void) {
	GLuint vertex_array;
	glGenVertexArrays(1, &vertex_array);
	glBindVertexArray(vertex_array);
	return vertex_array;
}

GLuint init_gpu_buffer(void) {
	GLuint buffer;
	glGenBuffers(1, &buffer);
	return buffer;
}

static void fail_on_shader_creation_error(const GLuint object_id,
	const ShaderCompilationStep compilation_step,
	void (*const creation_action) (const GLuint),
	void (*const log_length_getter) (const GLuint, const GLenum, GLint* const),
	void (*const log_getter) (const GLuint, const GLsizei, GLsizei* const, GLchar* const)) {

	creation_action(object_id);

	GLint log_length;
	log_length_getter(object_id, GL_INFO_LOG_LENGTH, &log_length);

	if (log_length > 0) {
		GLchar* const error_message = malloc((size_t) log_length + 1);
		log_getter(object_id, log_length, NULL, error_message);

		const byte compilation_step_id = (byte) compilation_step + 1;  // `fail` not used for this since the error message must be freed
		fprintf(stderr, "Shader creation step #%d - %s", compilation_step_id, error_message);
		free(error_message);
		exit(compilation_step_id);
	}
}

GLuint init_shader(const GLchar* const vertex_shader, const GLchar* const fragment_shader) {
	// In this, a sub-shader is a part of the big shader, like a vertex or fragment shader.
	const GLchar* const sub_shader_code[2] = {vertex_shader, fragment_shader};
	const GLenum sub_shader_types[2] = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
	GLuint sub_shaders[2], shader = glCreateProgram();

	for (ShaderCompilationStep step = CompileVertexShader; step < LinkShaders; step++) {
		const GLuint sub_shader = glCreateShader(sub_shader_types[step]);
		glShaderSource(sub_shader, 1, sub_shader_code + step, NULL);

		fail_on_shader_creation_error(sub_shader, step,
			glCompileShader, glGetShaderiv, glGetShaderInfoLog);

		glAttachShader(shader, sub_shader);

		sub_shaders[step] = sub_shader;
	}

	fail_on_shader_creation_error(shader, LinkShaders,
		glLinkProgram, glGetProgramiv, glGetProgramInfoLog);

	for (ShaderCompilationStep step = CompileVertexShader; step < LinkShaders; step++) {
		const GLuint sub_shader = sub_shaders[step];
		glDetachShader(shader, sub_shader);
		glDeleteShader(sub_shader);
	}

	return shader;
}

// `x` and `y` are top-down
byte sample_map_point(const byte* const map, const byte x, const byte y, const byte map_width) {
	return map[y * map_width + x];
}

const char* get_gl_error(void) {
	#define ERROR_CASE(error) case GL_##error: return #error;

	switch (glGetError()) {
		ERROR_CASE(NO_ERROR);
		ERROR_CASE(INVALID_ENUM);
		ERROR_CASE(INVALID_VALUE);
		ERROR_CASE(INVALID_OPERATION);
		ERROR_CASE(OUT_OF_MEMORY);
		ERROR_CASE(INVALID_FRAMEBUFFER_OPERATION);
		default: return "Unknown error";
	}

	#undef ERROR_CASE
}

#endif
