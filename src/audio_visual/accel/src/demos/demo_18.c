#include "../utils.c"
#include "../texture.c"
#include "../data/shaders.c"

// This demo gives a quake-like water effect

StateGL demo_18_init(void) {
	StateGL sgl = {.vertex_array = init_vao(), .num_vertex_buffers = 0, .num_textures = 1};

	sgl.shader_program = init_shader_program(water_vertex_shader, water_fragment_shader);
	glUseProgram(sgl.shader_program);

	sgl.num_textures = 1; // ../../../../assets/walls/sand.bmp, ../../../../assets/walls/hieroglyph.bmp, ../assets/water.bmp
	sgl.textures = malloc(sizeof(GLuint));
	*sgl.textures = init_plain_texture("../assets/lava.bmp", TexPlain, TexRepeating, OPENGL_DEFAULT_INTERNAL_PIXEL_FORMAT);
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
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

#ifdef DEMO_18
int main(void) {
	make_application(demo_18_drawer, demo_18_init, deinit_demo_vars);
}
#endif
