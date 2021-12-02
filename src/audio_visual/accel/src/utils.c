#ifndef UTILS_C
#define UTILS_C

#include "headers/utils.h"
#include "headers/constants.h"

Screen init_screen(const char* const title) {
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) fail("launch SDL", LaunchSDL);

	SDL_SetHintWithPriority(SDL_HINT_RENDER_VSYNC, "1", SDL_HINT_OVERRIDE);
	SDL_SetHintWithPriority(SDL_HINT_RENDER_BATCHING, "1", SDL_HINT_OVERRIDE);
	SDL_SetHintWithPriority(SDL_HINT_RENDER_DRIVER, "opengl", SDL_HINT_OVERRIDE);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, OPENGL_MAJOR_VERSION);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, OPENGL_MINOR_VERSION);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, DEPTH_BUFFER_BITS);

	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, MULTISAMPLE_SAMPLES);

	Screen screen = {
		.window = SDL_CreateWindow(title,
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			SCR_W, SCR_H, SDL_WINDOW_OPENGL)
	};

	screen.opengl_context = SDL_GL_CreateContext(screen.window);

	SDL_SetRelativeMouseMode(SDL_TRUE);
	SDL_WarpMouseInWindow(screen.window, SCR_W >> 1, SCR_H >> 1);

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) fail("initialize glew", LaunchGLEW);

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

void loop_application(const Screen* const screen, void (*const drawer)(const StateGL* const),
	StateGL (*const init)(void), void (*const deinit)(const StateGL* const)) {

	const int max_delay = 1000 / constants.fps;
	byte running = 1;
	SDL_Event event;
	const StateGL sgl = init();
	keys = SDL_GetKeyboardState(NULL);

	while (running) {
		const Uint32 before = SDL_GetTicks();

		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) running = 0;
		}

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		drawer(&sgl);
		if (keys[KEY_PRINT_OPENGL_ERROR]) GL_ERR_CHECK;
		SDL_GL_SwapWindow(screen -> window);

		const int wait = max_delay - (SDL_GetTicks() - before);
		if (wait > 0) SDL_Delay(wait);
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
		glDeleteTextures(sgl -> num_textures, sgl -> textures);
		free(sgl -> textures);
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
			NULL // Array buffer offset
		);
	}
	va_end(args);
}

static void fail_on_shader_creation_error(
	const GLuint object_id, const ShaderCompilationStep compilation_step,
	void (*const creation_action) (const GLuint),
	void (*const log_length_getter) (const GLuint, const GLenum, GLint* const),
	void (*const log_getter)(const GLuint, const GLsizei, GLsizei* const, GLchar* const)) {

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

GLuint init_shader_program(const char* const vertex_shader, const char* const fragment_shader) {
	const char* const shaders[2] = {vertex_shader, fragment_shader};
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

SDL_Surface* init_surface(const char* const path) {
	SDL_Surface* const surface = SDL_LoadBMP(path);
	if (surface == NULL) fail("open texture file", OpenImageFile);

	SDL_Surface* const converted_surface = SDL_ConvertSurfaceFormat(surface, SDL_PIXEL_FORMAT, 0);
	SDL_FreeSurface(surface);
	SDL_LockSurface(converted_surface);

	return converted_surface;
}

void deinit_surface(SDL_Surface* const surface) {
	SDL_UnlockSurface(surface);
	SDL_FreeSurface(surface);
}

// Expects that num_textures > 0. Params: path, texture wrap mode
GLuint* init_textures(const GLsizei num_textures, ...) {
	va_list args;
	va_start(args, num_textures);

	GLuint* const textures = malloc(num_textures * sizeof(GLuint));
	glGenTextures(num_textures, textures);

	for (int i = 0; i < num_textures; i++) {
		const char* const surface_path = va_arg(args, char*);
		const GLint texture_wrap_mode = va_arg(args, GLint);

		glBindTexture(GL_TEXTURE_2D, textures[i]);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, OPENGL_TEX_MAG_FILTER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, OPENGL_TEX_MIN_FILTER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texture_wrap_mode);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texture_wrap_mode);

		#ifdef ENABLE_ANISOTROPIC_FILTERING
		float aniso;
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &aniso);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
		#endif

		SDL_Surface* const surface = init_surface(surface_path);

		glTexImage2D(GL_TEXTURE_2D, 0, OPENGL_INTERNAL_PIXEL_FORMAT,
			surface -> w, surface -> h,
			0, OPENGL_INPUT_PIXEL_FORMAT, OPENGL_COLOR_CHANNEL_TYPE, surface -> pixels);

		glGenerateMipmap(GL_TEXTURE_2D);

		deinit_surface(surface);
	}

	va_end(args);
	return textures;
}

void select_texture_for_use(const GLuint texture, const GLuint shader_program) {
	const GLuint shader_texture_sampler = glGetUniformLocation(shader_program, "texture_sampler");
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture); // Set the current bound texture
	glUniform1i(shader_texture_sampler, 0); // Make the sampler read from texture unit 0
}

void enable_all_culling(void) {
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	// #if defined(DEMO_1) || defined(DEMO_2) || defined(DEMO_3) || defined(DEMO_5)
	glEnable(GL_CULL_FACE);
	// #endif
}

void draw_triangles(const GLsizei num_triangles) {
	glDrawArrays(GL_TRIANGLES, 0, num_triangles * 3);
}

static inline GLfloat to_radians(const GLfloat degrees) {
	return degrees * (GLfloat) M_PI / 180.0f;
}

#endif
