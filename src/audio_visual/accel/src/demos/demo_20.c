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

StateGL demo_20_init(void) {
	StateGL sgl = {.vertex_array = init_vao()};

	List billboards = init_list(2, Billboard);
	const Billboard first = {{2.5f, 3.5f, 4.5f}, 0}, second = {{0.0f, 1.0f, 2.0f}, 1};
	push_ptr_to_list(&billboards, &first);
	push_ptr_to_list(&billboards, &second);

	/*
	- In DrawableSet, put shader, texture set, a list of all billboards, and their indices
	- TODO: different billboard shader that can handle many
	- TODO: function init_billboard_list
	*/

	deinit_list(billboards);

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
