#ifndef UTILS_C
#define UTILS_C

#include "headers/utils.h"
#include "headers/texture.h"
#include "headers/constants.h"

Screen init_screen(const GLchar* const title) {
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) fail("launch SDL", LaunchSDL);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, OPENGL_MAJOR_VERSION);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, OPENGL_MINOR_VERSION);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);

	#ifdef USE_GAMMA_CORRECTION
	SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);
	#endif

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, DEPTH_BUFFER_BITS);

	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, MULTISAMPLE_SAMPLES);

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

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) fail("initialize glew", LaunchGLEW);

	SDL_GL_SetSwapInterval(
		#ifdef USE_VSYNC
		1
		#else
		0
		#endif
	);

	#ifdef USE_GAMMA_CORRECTION
	glEnable(GL_FRAMEBUFFER_SRGB);
	#endif

	keys = SDL_GetKeyboardState(NULL);

	return screen;
}

void deinit_screen(const Screen* const screen) {
	SDL_GL_DeleteContext(screen -> opengl_context);
	SDL_DestroyWindow(screen -> window);
	SDL_Quit();
}

void make_application(void (*const drawer)(const StateGL* const),
	StateGL (*const init)(void), void (*const deinit)(const StateGL* const)) {

	const Screen screen = init_screen("Accel Demo");

	printf("vendor = %s\nrenderer = %s\nversion = %s\n---\n",
		glGetString(GL_VENDOR), glGetString(GL_RENDERER), glGetString(GL_VERSION));

	loop_application(&screen, drawer, init, deinit);
	deinit_screen(&screen);
}

static void resize_window_if_needed(SDL_Window* const window) {
	static byte window_resized_last_tick = 0, window_is_fullscreen = 0, first_call = 1;
	static int desktop_width, desktop_height;

	if (first_call) {
		SDL_DisplayMode display_mode;
		SDL_GetDesktopDisplayMode(0, &display_mode);
		desktop_width = display_mode.w;
		desktop_height = display_mode.h;
		first_call = 0;
	}

	const byte resize_attempt = keys[constants.keys.toggle_fullscreen_window];

	if (!window_resized_last_tick && resize_attempt) {
		window_is_fullscreen = !window_is_fullscreen;
		window_resized_last_tick = 1;

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
	else if (!resize_attempt) window_resized_last_tick = 0;
}

static void set_triangle_fill_mode(void) {
	static byte in_triangle_fill_mode = 1, changed_mode_last_tick = 0;

	if (keys[KEY_TOGGLE_WIREFRAME_MODE]) {
		if (!changed_mode_last_tick) {
			in_triangle_fill_mode = !in_triangle_fill_mode;
			changed_mode_last_tick = 1;
			glPolygonMode(GL_FRONT_AND_BACK, in_triangle_fill_mode ? GL_FILL : GL_LINE);
		}
	}
	else changed_mode_last_tick = 0;
}

static byte application_should_exit(void) {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT)
			return 1;
	}

	/* On Ubuntu, the only way to activate the SDL_QUIT event type
	is by pressing the window exit button (not through pressing ctrl-w or ctrl-q!).
	That isn't possible for this application, since the mouse is locked in the window.
	Since exiting doesn't work normally, a ctrl key followed by an exit activation key
	serves as a manual workaround to this problem. */

	const byte // On Ubuntu, SDL_QUIT is not caught by SDL_PollEvent, so this circumvents that
		ctrl_key = keys[constants.keys.ctrl[0]] || keys[constants.keys.ctrl[1]],
		activate_exit_key = keys[constants.keys.activate_exit[0]] || keys[constants.keys.activate_exit[1]];

	return ctrl_key && activate_exit_key;
}

void loop_application(const Screen* const screen, void (*const drawer) (const StateGL* const),
	StateGL (*const init) (void), void (*const deinit) (const StateGL* const)) {

	#ifndef USE_VSYNC
	const GLfloat
		max_delay = 1000.0f / constants.fps,
		one_over_performance_freq = 1.0f / SDL_GetPerformanceFrequency();
	#endif

	byte running = 1;
	const StateGL sgl = init();

	while (running) {
		#ifndef USE_VSYNC
		const Uint64 before = SDL_GetPerformanceCounter();
		#endif

		running = !application_should_exit();

		resize_window_if_needed(screen -> window);
		set_triangle_fill_mode();

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		drawer(&sgl);

		if (keys[KEY_PRINT_OPENGL_ERROR]) GL_ERR_CHECK;
		if (keys[KEY_PRINT_SDL_ERROR]) SDL_ERR_CHECK;

		SDL_GL_SwapWindow(screen -> window);

		#ifndef USE_VSYNC
		const GLfloat ms_elapsed = (GLfloat) (SDL_GetPerformanceCounter() - before) * one_over_performance_freq * 1000.0f;
		const GLfloat wait_for_exact_fps = max_delay - ms_elapsed;
		if (wait_for_exact_fps > 12.0f) SDL_Delay(wait_for_exact_fps - 0.5f); // SDL_Delay tends to be late, so 0.5f accounts for that
		#endif
	}

	deinit(&sgl);
}

void deinit_demo_vars(const StateGL* const sgl) {
	glDeleteProgram(sgl -> shader_program);

	for (GLsizei i = 0; i < sgl -> num_vertex_buffers; i++) glDisableVertexAttribArray(i);

	if (sgl -> num_vertex_buffers > 0) {
		glDeleteBuffers(sgl -> num_vertex_buffers, sgl -> vertex_buffers);
		free(sgl -> vertex_buffers);
	}

	if (sgl -> num_textures > 0) {
		deinit_textures(sgl -> num_textures, sgl -> textures);
	}

	glDeleteVertexArrays(1, &sgl -> vertex_array);
}

GLuint init_vao(void) {
	GLuint vertex_array;
	glGenVertexArrays(1, &vertex_array);
	glBindVertexArray(vertex_array);
	return vertex_array;
}

// Buffer data ptr, size of buffer
GLuint* init_vbos(const GLsizei num_buffers, ...) {
	va_list args;
	va_start(args, num_buffers);

	GLuint* const vbos = malloc(num_buffers * sizeof(GLuint));
	glGenBuffers(num_buffers, vbos);

	for (GLsizei i = 0; i < num_buffers; i++) {
		const GLfloat* const buffer_data_ptr = va_arg(args, GLfloat*);
		const int size_of_buffer = va_arg(args, int);

		glBindBuffer(GL_ARRAY_BUFFER, vbos[i]);
		glBufferData(GL_ARRAY_BUFFER, size_of_buffer, buffer_data_ptr, GL_STATIC_DRAW);
	}

	va_end(args);
	return vbos;
}

// Num components for vbo
void bind_vbos_to_vao(const GLuint* const vbos, const GLsizei num_vbos, ...) {
	va_list args;
	va_start(args, num_vbos);

	for (GLsizei i = 0; i < num_vbos; i++) {
		glEnableVertexAttribArray(i);
		glBindBuffer(GL_ARRAY_BUFFER, vbos[i]);

		const int num_components = va_arg(args, int);

		glVertexAttribPointer(
			i, num_components, GL_FLOAT, // Attribute index i, component size, type
			GL_FALSE, 0, // Not normalized, stride
			(void*) 0 // Array buffer offset
		);
	}
	va_end(args);
}

static void fail_on_shader_creation_error(
	const GLuint object_id, const ShaderCompilationStep compilation_step,
	void (*const creation_action) (const GLuint),
	void (*const log_length_getter) (const GLuint, const GLenum, GLint* const),
	void (*const log_getter) (const GLuint, const GLsizei, GLsizei* const, GLchar* const)) {

	creation_action(object_id);

	GLint log_length;
	log_length_getter(object_id, GL_INFO_LOG_LENGTH, &log_length);

	if (log_length > 0) {
		GLchar* const error_message = malloc(log_length + 1);
		log_getter(object_id, log_length, NULL, error_message);

		const byte compilation_step_id = compilation_step + 1;
		fprintf(stderr, "Shader creation step #%d - %s", compilation_step_id, error_message);
		free(error_message);
		exit(compilation_step_id);
	}
}

GLuint init_shader_program(const GLchar* const vertex_shader, const GLchar* const fragment_shader) {
	const GLchar* const shaders[2] = {vertex_shader, fragment_shader};
	const GLenum gl_shader_types[2] = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
	GLuint shader_ids[2], program_id = glCreateProgram();

	for (ShaderCompilationStep step = CompileVertexShader; step < LinkShaders; step++) {
		shader_ids[step] = glCreateShader(gl_shader_types[step]);

		const GLuint shader_id = shader_ids[step];
		glShaderSource(shader_id, 1, shaders + step, NULL);

		fail_on_shader_creation_error(shader_id, step,
			glCompileShader, glGetShaderiv, glGetShaderInfoLog);

		glAttachShader(program_id, shader_id);
	}

	fail_on_shader_creation_error(program_id, LinkShaders,
		glLinkProgram, glGetProgramiv, glGetProgramInfoLog);

	for (ShaderCompilationStep step = CompileVertexShader; step < LinkShaders; step++) {
		const GLuint shader_id = shader_ids[step];
		glDetachShader(program_id, shader_id);
		glDeleteShader(shader_id);
	}

	return program_id;
}

void enable_all_culling(void) {
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
}

// X and Y are top-down
byte* map_point(byte* const map, const byte x, const byte y, const byte map_width) {
	return map + (y * map_width + x);
}

const char* get_gl_error(void) {
	#define ERROR_CASE(error) case error: return #error;

	switch (glGetError()) {
		ERROR_CASE(GL_NO_ERROR);
		ERROR_CASE(GL_INVALID_ENUM);
		ERROR_CASE(GL_INVALID_VALUE);
		ERROR_CASE(GL_INVALID_OPERATION);
		ERROR_CASE(GL_INVALID_FRAMEBUFFER_OPERATION);
		ERROR_CASE(GL_STACK_UNDERFLOW);
		ERROR_CASE(GL_STACK_OVERFLOW);
		default: return "Unknown error";
	}

	#undef ERROR_CASE
}

#endif
