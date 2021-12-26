#include "../utils.c"
#include "../billboard.c"
#include "../texture.c"
#include "../skybox.c"
#include "../camera.c"
#include "../batch_draw_context.c"

// Batched + culled billboard drawing

/*
- To begin with, just draw all in one big unbatched buffer
- Batching after
- Only using instancing for glVertexAttribDivisor
*/

typedef struct {
	BatchDrawContext billboard_draw_context;
	Skybox skybox;
} SceneState;

StateGL demo_20_init(void) {
	StateGL sgl = {.vertex_array = init_vao(), .num_vertex_buffers = 0, .num_textures = 0};

	/* How would I update indices for a texture index? Perhaps mod it by the current time in some way;
	Or actually not b/c the texture index may also depend on an enemy state */

	BatchDrawContext draw_context = init_billboard_draw_context(
		1,
		(Billboard) {0, {2.0f, 2.0f}, {0.0f, 0.0f, 0.0f}}

		/*
		(Billboard) {1, {1.0f, 1.0f}, {0.0f, 1.0f, 1.0f}},
		(Billboard) {2, {1.0f, 1.0f}, {0.0f, 1.0f, 2.0f}},
		(Billboard) {3, {1.0f, 1.0f}, {0.0f, 1.0f, 3.0f}},
		(Billboard) {4, {1.0f, 1.0f}, {0.0f, 1.0f, 4.0f}},
		(Billboard) {5, {1.0f, 1.0f}, {0.0f, 1.0f, 5.0f}},
		(Billboard) {6, {1.0f, 1.0f}, {0.0f, 1.0f, 6.0f}},
		(Billboard) {7, {1.0f, 1.0f}, {0.0f, 1.0f, 7.0f}},
		(Billboard) {8, {1.0f, 1.0f}, {0.0f, 1.0f, 8.0f}},
		(Billboard) {9, {1.0f, 1.0f}, {0.0f, 1.0f, 9.0f}}
		*/
	);

	draw_context.texture_set = init_texture_set(TexNonRepeating, 3, 2, 512, 512,
		// "../../../../assets/objects/hot_dog.bmp",
		"../../../../assets/walls/hieroglyphics.bmp",

		"../../../../assets/objects/teleporter.bmp",
		"../../../../assets/objects/robot.bmp",

		"../../../../assets/spritesheets/metroid.bmp", 2, 2, 4,
		"../../../../assets/spritesheets/bogo.bmp", 2, 3, 6
	);

	SceneState* const scene_state = malloc(sizeof(SceneState));
	*scene_state = (SceneState) {draw_context, init_skybox("../assets/desert.bmp")};
	sgl.any_data = scene_state;

	enable_all_culling();

	return sgl;
}

void demo_20_drawer(const StateGL* const sgl) {
	const SceneState* const scene_state = (SceneState*) sgl -> any_data;

	static byte first_call = 1;
	static Camera camera;

	if (first_call) {
		init_camera(&camera, (vec3) {0.0f, 0.0f, 0.0f}); // 0.0f, 1.5, -2.5f
		first_call = 0;
	}

	update_camera(&camera, get_next_event());
	draw_skybox(scene_state -> skybox, &camera);
	draw_billboards(&scene_state -> billboard_draw_context, &camera);

	//////////

	/*
	const Billboard hot_dog = ((Billboard*) (scene_state -> billboard_draw_context.object_buffers.cpu.data))[0];
	vec4 frustum_planes[6];
	glm_frustum_planes((vec4*) camera.view_projection, frustum_planes);
	DEBUG(billboard_in_view_frustum(hot_dog, frustum_planes), d);
	*/
}

void demo_20_deinit(const StateGL* const sgl) {
	const SceneState* const scene_state = (SceneState*) sgl -> any_data;
	deinit_skybox(scene_state -> skybox);
	deinit_batch_draw_context(&scene_state -> billboard_draw_context, 0);
	free(sgl -> any_data);
	deinit_demo_vars(sgl);
}

#ifdef DEMO_20
int main(void) {
	make_application(demo_20_drawer, demo_20_init, demo_20_deinit);
}
#endif
