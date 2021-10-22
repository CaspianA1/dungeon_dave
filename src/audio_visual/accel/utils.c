#ifndef UTILS_C
#define UTILS_C
#include "utils.h"

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

void make_application(void (*const drawer)(const StateGL),
	StateGL (*const init)(void), void (*const deinit)(StateGL)) {

	const Screen screen = init_screen("Accel Demo");

	printf("vendor = %s\nrenderer = %s\nversion = %s\n---\n",
		glGetString(GL_VENDOR), glGetString(GL_RENDERER), glGetString(GL_VERSION));

	loop_application(&screen, drawer, init, deinit, 60);
	deinit_screen(&screen);
}

void loop_application(const Screen* const screen, void (*const drawer)(const StateGL),
	StateGL (*const init)(void), void (*const deinit)(StateGL), const byte fps) {

	const double max_delay = 1000.0 / fps;
	byte running = 1;
	SDL_Event event;
	const StateGL sgl = init();

	while (running) {
		const Uint32 before = SDL_GetTicks();

		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) running = 0;
		}

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		drawer(sgl);
		SDL_GL_SwapWindow(screen -> window);

		const int wait = max_delay - (SDL_GetTicks() - before);
		if (wait > 0) SDL_Delay(wait);
	}

	deinit(sgl);
}

GLuint init_shader_program(const char* const vertex_shader, const char* const fragment_shader) {
	typedef enum {Vertex, Fragment} ShaderType;

	const char* const shaders[2] = {vertex_shader, fragment_shader};
	const GLenum gl_shader_types[2] = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
	GLuint shader_ids[2], program_id = glCreateProgram();

	for (ShaderType type = 0; type < 2; type++) {
		shader_ids[type] = glCreateShader(gl_shader_types[type]);

		const GLuint shader_id = shader_ids[type];
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
		const GLuint shader_id = shader_ids[type];
		glDetachShader(program_id, shader_id);
		glDeleteShader(shader_id);
	}

	glUseProgram(program_id);
	return program_id;
}

void bind_vbos_to_vao(const GLuint* const vbos, const int num_vbos) {
	for (int i = 0; i < num_vbos; i++) {
		glEnableVertexAttribArray(i);
		glBindBuffer(GL_ARRAY_BUFFER, vbos[i]);

		glVertexAttribPointer(
			i, 3, GL_FLOAT, // attribute i, num points for a vertex, type
			GL_FALSE, 0, // not normalized, stride
			NULL // array buffer offset
		);
	}
}

void unbind_vbos_from_vao(const int num_vbos) {
	for (int i = 0; i < num_vbos; i++) glDisableVertexAttribArray(i);
}

GLuint init_vao(void) {
	GLuint vertex_array;
	glGenVertexArrays(1, &vertex_array);
	glBindVertexArray(vertex_array);
	return vertex_array;
}

// buffer data ptr, number of points in buffer
GLuint* init_vbos(const int num_buffers, ...) {
	va_list args;
	va_start(args, num_buffers);

	GLuint* const vbos = malloc(num_buffers * sizeof(GLuint));
	glGenBuffers(num_buffers, vbos);

	for (int i = 0; i < num_buffers; i++) {
		const GLfloat* const buffer_data_ptr = va_arg(args, GLfloat*);
		const int num_points_in_buffer = va_arg(args, int);

		glBindBuffer(GL_ARRAY_BUFFER, vbos[i]);
		glBufferData(GL_ARRAY_BUFFER, num_points_in_buffer * sizeof(GLfloat), buffer_data_ptr, GL_STATIC_DRAW);
	}

	va_end(args);
	return vbos;
}

GLuint init_texture(const char* const path) {
	SDL_Surface* const surface = SDL_LoadBMP(path);
	if (surface == NULL) fail("open texture file", OpenImageFile);

	SDL_Surface* const converted_surface = SDL_ConvertSurfaceFormat(surface, SDL_PIXEL_FORMAT, 0);
	SDL_FreeSurface(surface);
	SDL_LockSurface(converted_surface);

	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexImage2D(GL_TEXTURE_2D, 0, OPENGL_PIXEL_FORMAT,
		converted_surface -> w, converted_surface -> h,
		0, OPENGL_PIXEL_FORMAT, OPENGL_COLOR_CHANNEL_TYPE, converted_surface -> pixels);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, OPENGL_TEX_MAG_FILTER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, OPENGL_TEX_MIN_FILTER);
	glGenerateMipmap(GL_TEXTURE_2D);

	SDL_UnlockSurface(converted_surface);
	SDL_FreeSurface(converted_surface);

	return texture;
}

void deinit_demo_vars(const StateGL sgl) {
	glDeleteProgram(sgl.shader_program);

	glDeleteBuffers(sgl.num_vertex_buffers, sgl.vertex_buffers);
	free(sgl.vertex_buffers);

	glDeleteVertexArrays(1, &sgl.vertex_array);
}

#define draw_from_bound_vbo(num_vertices) glDrawArrays(GL_TRIANGLES, 0, num_vertices)

#endif
