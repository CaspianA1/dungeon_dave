#include "../utils.c"
#include "../data/shaders.c"

// This demo aims to make a quake-like water effect
// https://forum.unity.com/threads/quake-1-liquid-shader.873616/

StateGL demo_18_init(void) {
	StateGL sgl = {.vertex_array = init_vao()};

	// bl, br, tl -> br, tl, tr
	const GLbyte full_screen_quad[] = {
		-1, -1, // Bottom left
		1, -1, // Bottom right
		-1, 1, // Top left
		1, 1 // Top right
	};

	sgl.num_vertex_buffers = 1;
	sgl.vertex_buffers = init_vbos(sgl.num_vertex_buffers, full_screen_quad, sizeof(full_screen_quad));

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_BYTE, GL_FALSE, 0, NULL);

	sgl.shader_program = init_shader_program(water_vertex_shader, water_fragment_shader);
	glUseProgram(sgl.shader_program);

	sgl.num_textures = 1;
	sgl.textures = init_textures(sgl.num_textures, "../../../assets/walls/hieroglyph.bmp", tex_nonrepeating);
	select_texture_for_use(sgl.textures[0], sgl.shader_program);

	return sgl;
}

void demo_18_drawer(const StateGL* const sgl) {
	// Pass in 2 angles

	(void) sgl;
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

#ifdef DEMO_18
int main(void) {
	make_application(demo_18_drawer, demo_18_init, deinit_demo_vars);
}
#endif
