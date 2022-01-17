#include "../utils.c"
#include "../data/shaders.c"

// Perlin noise!

const char *const perlin_vertex_shader =
	"#version 330 core\n"

	// Bottom left, bottom right, top left, top right
	"const vec2 corners[4] = vec2[4] (\n"
		"vec2(-1, -1), vec2(1, -1), vec2(-1, 1), vec2(1, 1)\n"
	");\n"

	"void main(void) {\n"
		"gl_Position = vec4(corners[gl_VertexID], 0.0f, 1.0f);\n"
	"}\n",

// https://www.shadertoy.com/view/4lB3zz
*const perlin_fragment_shader =
	"#version 330 core\n"

	"out vec3 color;\n"

	"uniform int choice, first_octave, octaves;\n"
	"uniform float rand_factor, persistence;\n"
	"uniform vec2 screen_size;\n"

	"float noise(int x, int y) {\n"
		"return 2.0f * fract(sin(rand_factor * dot(vec2(x, y), vec2(12.9898f, 78.233f))) * 43758.5453f) - 1.0f;\n"
	"}\n"

	// Divs to muls, and sum 3x3 mat?
	"float noise_from_samples(int cx, int cy, mat4 samples) {\n" // Pass by reference?
		"return samples[cy][cx] / 4.0f\n"
			"+ (samples[cy - 1][cx] + samples[cy + 1][cx] + samples[cy][cx - 1] + samples[cy][cx + 1]) / 8.0f\n"
			"+ (samples[cy - 1][cx - 1] + samples[cy - 1][cx + 1] + samples[cy + 1][cx - 1] + samples[cy + 1][cx + 1]) / 16.0f;\n"
	"}\n"

	"float lerp_noise(vec2 pos) {\n"
		"int ix = int(pos.x), iy = int(pos.y);\n"
		"vec2 fractions = fract(pos);\n"

		"mat4 samples = mat4(\n" // This is in row-major order
			"noise(ix - 1, iy - 1), noise(ix, iy - 1), noise(ix + 1, iy - 1), noise(ix + 2, iy - 1),\n"
			"noise(ix - 1, iy),     noise(ix, iy),     noise(ix + 1, iy),     noise(ix + 2, iy),\n"
			"noise(ix - 1, iy + 1), noise(ix, iy + 1), noise(ix + 1, iy + 1), noise(ix + 2, iy + 1),\n"
			"noise(ix - 1, iy + 2), noise(ix, iy + 2), noise(ix + 1, iy + 2), noise(ix + 2, iy + 2)\n"
		");\n"

		"float\n"
			"noise_top = mix(noise_from_samples(1, 1, samples), noise_from_samples(2, 1, samples), fractions.x),\n"
			"noise_bottom = mix(noise_from_samples(1, 2, samples), noise_from_samples(2, 2, samples), fractions.x);\n"

		"return mix(noise_top, noise_bottom, fractions.y);\n"
	"}\n"

	"float perlin_noise_2D(vec2 pos) {\n"
		"float sum = 0.0f, amplitude = pow(persistence, first_octave);\n"
		"int frequency = 2 << (first_octave - 1);\n"
		"for (int i = first_octave; i < octaves + first_octave; i++, frequency <<= 1, amplitude *= persistence)\n"
			"sum += lerp_noise(pos * frequency) * amplitude;\n"
		"return sum;\n"
	"}\n"

	"void main(void) {\n"
		"vec2 uv = gl_FragCoord.xy / screen_size.xy;\n"
		"color = vec3(0.3f + perlin_noise_2D(uv));\n"
	"}\n";

StateGL demo_18_init(void) {
	StateGL sgl = {.vertex_array = init_vao(), .num_vertex_buffers = 0, .num_textures = 0};

	sgl.shader_program = init_shader_program(perlin_vertex_shader, perlin_fragment_shader);
	glUseProgram(sgl.shader_program);

	return sgl;
}

void demo_18_drawer(const StateGL* const sgl) {
	static GLint choice_id, first_octave_id, octaves_id, rand_factor_id, persistence_id;
	static byte first_call = 1;

	if (first_call) {
		const GLuint shader = sgl -> shader_program;
		choice_id = glGetUniformLocation(shader, "choice");
		first_octave_id = glGetUniformLocation(shader, "first_octave");
		octaves_id = glGetUniformLocation(shader, "octaves");
		rand_factor_id = glGetUniformLocation(shader, "rand_factor");
		persistence_id = glGetUniformLocation(shader, "persistence");
		glUniform2f(glGetUniformLocation(shader, "screen_size"), WINDOW_W, WINDOW_H);
		first_call = 0;
	}

	static GLfloat rand_factor = 1.0f;
	const GLfloat step = 0.00001f;
	if (keys[SDL_SCANCODE_T]) rand_factor += step;
	if (keys[SDL_SCANCODE_Y]) rand_factor -= step;

	glUniform1i(choice_id, keys[SDL_SCANCODE_C]);
	glUniform1i(first_octave_id, 3);
	glUniform1i(octaves_id, 8);
	glUniform1f(rand_factor_id, rand_factor);
	glUniform1f(persistence_id, 0.6f);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

#ifdef DEMO_21
int main(void) {
	make_application(demo_18_drawer, demo_18_init, deinit_demo_vars);
}
#endif
