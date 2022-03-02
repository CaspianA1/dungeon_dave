// Another attempt at shadow maps

#include "../utils.c"
#include "../list.c"
#include "../batch_draw_context.c"
#include "../texture.c"
#include "../data/maps.c"
#include "../sector.c"
#include "../camera.c"
#include "../event.c"
#include "../shadow_map.c"

//////////

typedef struct {
	List sectors;
	BatchDrawContext sector_draw_context;
	ShadowMapContext shadow_map_context;

	const byte* const heightmap;
	const byte map_size[2];
} SceneState;

//////////

StateGL demo_24_init(void) {
	StateGL sgl = {.vertex_array = init_vao(), .num_vertex_buffers = 0, .num_textures = 0};

	SceneState scene_state = {
		.shadow_map_context = init_shadow_map_context(4096, 4096,
			(vec3) {3.141779f, 5.575195f, 12.794771f},
			(vec3) {0.495601f, -0.360811f, -0.790060f},
			(vec3) {0.0f, 1.0f, 0.0f}
		),

		.heightmap = (byte*) palace_heightmap,
		.map_size = {palace_width, palace_height}
	};

	init_sector_draw_context(&scene_state.sector_draw_context, &scene_state.sectors,
		scene_state.heightmap, (byte*) palace_texture_id_map, scene_state.map_size);

	scene_state.sector_draw_context.texture_set = init_texture_set(TexRepeating,
		11, 0, 128, 128,
		"../../../../assets/walls/sand.bmp", "../../../../assets/walls/pyramid_bricks_4.bmp",
		"../../../../assets/walls/marble.bmp", "../../../../assets/walls/hieroglyph.bmp",
		"../../../../assets/walls/window.bmp", "../../../../assets/walls/saqqara.bmp",
		"../../../../assets/walls/sandstone.bmp", "../../../../assets/walls/cobblestone_3.bmp",
		"../../../../assets/walls/horses.bmp", "../../../../assets/walls/mesa.bmp",
		"../../../../assets/walls/arthouse_bricks.bmp"
	);

	sgl.any_data = malloc(sizeof(SceneState));
	memcpy(sgl.any_data, &scene_state, sizeof(SceneState));

	enable_all_culling();
	glEnable(GL_MULTISAMPLE);
	glClearColor(0.4f, 0.4f, 0.4f, 0.0f); // Light gray

	return sgl;
}

void demo_24_drawer(const StateGL* const sgl) {
	SceneState* const scene_state = (SceneState*) sgl -> any_data;

	const BatchDrawContext* const draw_context = &scene_state -> sector_draw_context;
	ShadowMapContext* const shadow_map_context = &scene_state -> shadow_map_context;

	static Camera camera;
	static byte first_call = 1;

	if (first_call) {
		init_camera(&camera, (vec3) {1.5f, 1.5f, 1.5f});
		first_call = 0;
	}

	const Event event = get_next_event();
	update_camera(&camera, event, NULL);

	//////////

	if (keys[SDL_SCANCODE_C]) {
		memcpy(shadow_map_context -> light_context.pos, camera.pos, sizeof(vec3));
		memcpy(shadow_map_context -> light_context.dir, camera.dir, sizeof(vec3));
		memcpy(shadow_map_context -> light_context.up, camera.up, sizeof(vec3));
		render_all_sectors_to_shadow_map(shadow_map_context, draw_context, event.screen_size);
	}

	draw_visible_sectors(draw_context, shadow_map_context, &scene_state -> sectors, &camera);
}

void demo_24_deinit(const StateGL* const sgl) {
	const SceneState* const scene_state = (SceneState*) sgl -> any_data;

	deinit_batch_draw_context(&scene_state -> sector_draw_context);
	deinit_list(scene_state -> sectors);
	deinit_shadow_map_context(&scene_state -> shadow_map_context);

	free(sgl -> any_data);

	deinit_demo_vars(sgl);
}

#ifdef DEMO_24
int main(void) {
	make_application(demo_24_drawer, demo_24_init, demo_24_deinit);
}
#endif
