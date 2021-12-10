#include "../utils.c"
#include "../list.c"
#include "../headers/buffer_defs.h"

#include "../drawable_set.c"

// Batched + culled billboard drawing

typedef struct {
	bb_pos_component_t pos[3];
	const bb_texture_id_t texture_id; // Max is 65536
	// 2 bytes of padding
} Billboard;

// Num billboards, billboards
DrawableSet init_billboard_list(const size_t num_billboards, ...) {
	DrawableSet billboard_list = {0};
	// Def cpu and gpu buffer

	va_list args;
	va_start(args, num_billboards);

	for (size_t i = 0; i < num_billboards; i++) {
		Billboard billboard = va_arg(args, Billboard);
		(void) billboard;
	}

	va_end(args);
	return billboard_list;
}

StateGL demo_20_init(void) {
	StateGL sgl = {.vertex_array = init_vao()};

	DrawableSet billboard_list = init_billboard_list(2,
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
