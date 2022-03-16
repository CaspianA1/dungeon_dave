#include "../utils.c"
#include "../texture.c"
#include "../headers/buffer_defs.h"
#include "../data/shaders.c"

// This demo gives a quake-like water effect

const GLchar *const water_vertex_shader =
	"#version 330 core\n"
	"#define sin_cycles 6.28f\n"
	"#define speed 2.0f\n"

	"out vec2 fragment_UV, angles;\n"

	"uniform float time;\n" // In seconds

	// Bottom left, bottom right, top left, top right
	"const vec2 screen_corners[4] = vec2[4](\n"
		"vec2(-1.0f, -1.0f), vec2(1.0f, -1.0f),\n"
		"vec2(-1.0f, 1.0f), vec2(1.0f, 1.0f)\n"
	");\n"

	"void main(void) {\n"
		"gl_Position = vec4(screen_corners[gl_VertexID], 0.0f, 1.0f);\n"
		"fragment_UV = gl_Position.xy * vec2(0.5f, -0.5f) + 0.5f;\n"
		"angles = fragment_UV.yx * sin_cycles + time * speed;\n"
	"}\n",

*const water_fragment_shader =
	"#version 330 core\n"
	"#define dist_mag 0.05f\n"

	"in vec2 fragment_UV, angles;\n"

	"out vec3 color;\n"

	"uniform sampler2D texture_sampler;\n"

	"void main(void) {\n"
		"vec2 wavy_UV = fragment_UV + sin(angles) * dist_mag;\n"
		"color = texture(texture_sampler, wavy_UV).rgb;\n"
	"}\n";

StateGL demo_18_init(void) {
	StateGL sgl = {.vertex_array = init_vao(), .num_vertex_buffers = 0, .num_textures = 1};

	sgl.shader_program = init_shader_program(water_vertex_shader, water_fragment_shader);
	glUseProgram(sgl.shader_program);

	sgl.num_textures = 1; // ../../../../assets/walls/sand.bmp, ../../../../assets/walls/hieroglyph.bmp, ../assets/water.bmp
	sgl.textures = malloc(sizeof(GLuint));
	*sgl.textures = init_plain_texture("../assets/lava.bmp", TexPlain, TexRepeating,
		TexLinear, TexLinearMipmapped, OPENGL_DEFAULT_INTERNAL_PIXEL_FORMAT);

	use_texture(*sgl.textures, sgl.shader_program, "texture_sampler", TexPlain, 0);

	return sgl;
}

void demo_18_drawer(const StateGL* const sgl) {
	static GLint time_id;
	static bool first_call = true;

	if (first_call) {
		INIT_UNIFORM(time, sgl -> shader_program);
		first_call = false;
	}

	UPDATE_UNIFORM(time, 1f, SDL_GetTicks() / 1000.0f);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, corners_per_quad);
}

#ifdef DEMO_18
int main(void) {
	make_application(demo_18_drawer, demo_18_init, deinit_demo_vars);
}
#endif
