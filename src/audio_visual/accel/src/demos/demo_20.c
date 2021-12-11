#include "../utils.c"
#include "../list.c"
#include "../drawable_set.c"
#include "../texture.c"

// Batched + culled billboard drawing
// To begin with, just draw all of them in one big unbatched buffer

typedef struct {
	bb_pos_component_t pos[3];
	const bb_texture_id_t texture_id; // Max is 65536
	// 2 bytes of padding
} Billboard;

// Billboards
DrawableSet init_billboard_list(const GLuint texture_set, const size_t num_billboards, ...) {
	DrawableSet billboard_list = {
		.objects = init_list(num_billboards, Billboard),
		.object_indices = init_list(num_billboards, buffer_index_t), // 1 index per billboard attribute set
		.shader = 0, // init_shader_program("", ""),
		.texture_set = texture_set
	};

	/*
	- vbo not used here
	- use dbo as ubo
	- ibo used, for batching
	- get mapped ibo pointer

	- actually, to gpu, pass just uniforms that stay the same across every 4 vertices
	- how do ubos work, and are they right for this?
	*/

	va_list args;
	va_start(args, num_billboards);

	for (size_t i = 0; i < num_billboards; i++) {
		Billboard billboard = va_arg(args, Billboard);

		printf("{{%lf, %lf, %lf}, %d}\n",
			(double) billboard.pos[0], (double) billboard.pos[1],
			(double) billboard.pos[2], billboard.texture_id);
	}

	va_end(args);
	return billboard_list;
}

StateGL demo_20_init(void) {
	StateGL sgl = {.vertex_array = init_vao(), .num_vertex_buffers = 0, .num_textures = 0};

	GLuint texture_set = init_texture_set(TexNonRepeating,
		64, 64, 3, "../../../assets/objects/tomato.bmp",
		"../../../assets/objects/teleporter.bmp",
		"../../../assets/objects/robot.bmp"
	);

	DrawableSet billboard_list = init_billboard_list(
		texture_set, 2,
		(Billboard) {{2.5f, 3.5f, 4.5f}, 0},
		(Billboard) {{0.0f, 1.0f, 2.0f}, 1}
	);

	(void) billboard_list;
	puts("Now do something with the billboard list");


	/*
	- In DrawableSet, put shader, texture set, a list of all billboards, and their indices
	- TODO: different billboard shader that can handle many billboards as a stream
	- TODO: function init_billboard_list
	*/

	return sgl;
}

void demo_20_drawer(const StateGL* const sgl) {
	(void) sgl;
}

#ifdef DEMO_20
int main(void) {
	make_application(demo_20_drawer, demo_20_init, deinit_demo_vars);
}
#endif
