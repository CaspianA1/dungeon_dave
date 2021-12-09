#include "../utils.c"
#include "../camera.c"
#include "../data/shaders.c"
#include "../skybox.c"

// http://www.humus.name/index.php?page=Textures

StateGL demo_15_init(void) {
	StateGL sgl = {.vertex_array = init_vao()};

	sgl.num_vertex_buffers = 0;
	sgl.num_textures = 0;

	const Skybox skybox = init_skybox("assets/wadi.bmp");
	sgl.any_data = malloc(sizeof(Skybox));
	memcpy(sgl.any_data, &skybox, sizeof(Skybox));

	glEnableVertexAttribArray(0);

	return sgl;
}

void demo_15_deinit(const StateGL* const sgl) {
	deinit_skybox(*(Skybox*) sgl -> any_data);
	deinit_demo_vars(sgl);
}

void demo_15_drawer(const StateGL* const sgl) {
	static Camera camera;
	static byte first_call = 1;

	if (first_call) {
		init_camera(&camera, (vec3) {0.0f, 0.0f, 0.0f});
		first_call = 0;
	}

	update_camera(&camera, get_next_event());
	draw_skybox(*(Skybox*) sgl -> any_data, &camera);
}

#ifdef DEMO_15
int main(void) {
	make_application(demo_15_drawer, demo_15_init, demo_15_deinit);
}
#endif
