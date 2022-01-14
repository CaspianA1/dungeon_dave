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

	"uniform float rand_factor;\n"
	"uniform vec2 screen_size;\n"

	"const int first_octave = 3, octaves = 8;\n"
	"const float persistence = 0.6f;\n"

	"float noise(int x, int y) {\n"
		"return 2.0f * fract(sin(rand_factor * dot(vec2(x, y), vec2(12.9898f, 78.233f))) * 43758.5453f) - 1.0f;\n"
	"}\n"

	"float smooth_noise(int x, int y) {\n"
		"return noise(x, y) / 4.0f\n"
			"+ (noise(x + 1, y) + noise(x - 1, y) + noise(x, y + 1) + noise(x, y - 1)) / 8.0f\n"
			"+ (noise(x + 1, y + 1) + noise(x + 1, y - 1) + noise(x - 1, y + 1) + noise(x - 1, y - 1)) / 16.0f;\n"
	"}\n"

	"float cos_lerp(float x, float y, float n) {\n"
		"float r = n * 3.1415926536f;\n"
		"float f = (1.0f - cos(r)) * 0.5f;\n"
		"return x * (1.0 - f) + y * f;\n"
	"}\n"

	"float lerp_noise(vec2 pos) {\n"
		"int ix = int(pos.x), iy = int(pos.y);\n"

		"vec2 fractions = fract(pos);\n"

		"float\n"
			"noise_top = cos_lerp(smooth_noise(ix, iy), smooth_noise(ix + 1, iy), fractions.x),\n"
			"noise_bottom = cos_lerp(smooth_noise(ix, iy + 1), smooth_noise(ix + 1, iy + 1), fractions.x);\n"

		"return cos_lerp(noise_top, noise_bottom, fractions.y);\n"
	"}\n"

	"float perlin_noise_2D(vec2 pos) {\n"
		"float sum = 0.0f;\n"
		"for (int i = first_octave; i < octaves + first_octave; i++) {\n"
			"int frequency = 2 << (i - 1);\n"
			"float amplitude = pow(persistence, i);\n"
			"sum += lerp_noise(pos * frequency) * amplitude;\n"
		"}\n"
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
	static GLint rand_factor_id;
	static byte first_call = 1;

	if (first_call) {
		const GLuint shader = sgl -> shader_program;
		rand_factor_id = glGetUniformLocation(shader, "rand_factor");
		glUniform2f(glGetUniformLocation(shader, "screen_size"), WINDOW_W, WINDOW_H);
		first_call = 0;
	}

	static GLfloat rand_factor = 1.0f;
	const GLfloat step = 0.00001f;
	if (keys[SDL_SCANCODE_T]) rand_factor += step;
	if (keys[SDL_SCANCODE_Y]) rand_factor -= step;
	glUniform1f(rand_factor_id, rand_factor);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

#ifdef DEMO_21
int main(void) {
	make_application(demo_18_drawer, demo_18_init, deinit_demo_vars);
}
#endif
