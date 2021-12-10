#include "../utils.c"
#include "../list.c"

// Batched + culled billboard drawing

typedef GLfloat billboard_pos_component_t;

typedef struct {
	billboard_pos_component_t pos[3];
	const byte texture_id; // Max is 255 here (includes spritesheet indices and such)
} Billboard;

StateGL demo_20_init(void) {
	StateGL sgl = {.vertex_array = init_vao()};

	List billboards = init_list(2, Billboard);

	const Billboard
		first = {{2.5f, 3.5f, 4.5f}, 0},
		second = {{0.0f, 1.0f, 2.0f}, 1};

	push_ptr_to_list(&billboards, &first);
	push_ptr_to_list(&billboards, &second);
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
