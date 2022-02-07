/*
- Shadow maps for this demo - no shadow volumes, after some thinking.
- Start with making a plain shadow map: https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
- After that, move onto an omnidirectional shadow map: https://learnopengl.com/Advanced-Lighting/Shadows/Point-Shadows
*/

#include "../utils.c"

StateGL demo_23_init(void) {
	StateGL sgl = {.vertex_array = init_vao(), .num_vertex_buffers = 0, .num_textures = 0};
	return sgl;
}

void demo_23_drawer(const StateGL* const sgl) {
	(void) sgl;
}

void demo_23_deinit(const StateGL* const sgl) {
	deinit_demo_vars(sgl);
}

#ifdef DEMO_23
int main(void) {
	make_application(demo_23_drawer, demo_23_init, demo_23_deinit);
}
#endif
