#include "../utils.c"
#include "../skybox.c"
#include "../data/maps.c"

#include "../sector.c"
#include "../billboard.c"
#include "../camera.c"

/*
- NEXT: new_map back part + a texmap for it
- NEXT 2: a bounding volume hierarchy, through metasector trees (alloc through node pool)
- NEXT 3: looping animations for billboards
- NEXT 4: billboards that don't turn to face the player (just static ones); defined by center, size, and normal
- NEXT 5: Fix movement physics (one example: at FPS 10, can't jump over a block) (also, both bob and movement are stuttery - framerate spikes)

- A map maker. An init file that specifies textures and dimensions; draw/erase modes, export, and choose heights and textures
- More efficiently set statemap bit ranges
- For terrain, some objects popping out for half seconds
- Demo 12 pops a bit in the beginning, and demo 17 a bit less
- Camera var names to yaw, pitch, and roll (maybe)
- Billboard lighting that matches the sector lighting
- Base darkest distance of attenuated light on the world size
- Weird framerate dips in demo 17 (fix by doing gpu timing)
- Generic drawing setup for batch draw contexts
- Check the framerate via gpu timing (it's most likely much lower)

- Blit 2D sprite to whole screen
- Blit color rect to screen
- Then, flat weapon (that comes after physics)
- A dnd-styled font that's monospaced
- Use more of the cglm functions in `update_camera`, or make my own

- In the end, 5 shaders + accel components: sectors, billboards, skybox, weapon, ui elements
*/

typedef struct {
	List sectors;
	BatchDrawContext sector_draw_context, billboard_draw_context;
	Skybox skybox;
	byte* heightmap, map_size[2];
} SceneState;

StateGL demo_17_init(void) {
	/*
	(byte*) palace_heightmap, (byte*) palace_texture_id_map, palace_width, palace_height
	(byte*) pyramid_heightmap, (byte*) pyramid_texture_id_map, pyramid_width, pyramid_height
	(byte*) tpt_heightmap, (byte*) tpt_texture_id_map, tpt_width, tpt_height
	(byte*) new_heightmap, (byte*) texture_id_map, new_width, new_height
	(byte*) terrain_heightmap, (byte*) texture_id_map, terrain_width, terrain_height
	(byte*) tiny_heightmap, (byte*) tiny_heightmap, tiny_width, tiny_height
	(byte*) level_one_heightmap, (byte*) level_one_texture_id_map, level_one_width, level_one_height
	(byte*) checker_heightmap, (byte*) texture_id_map, checker_width, checker_height
	*/

	StateGL sgl = {.vertex_array = init_vao(), .num_vertex_buffers = 0, .num_textures = 0};
	SceneState scene_state = {.skybox = init_skybox("../assets/wadi_upscaled.bmp"),
		.heightmap = (byte*) palace_heightmap, .map_size = {palace_width, palace_height}};

	//////////
	// static byte texture_id_map[terrain_height][terrain_width];
	init_sector_draw_context(&scene_state.sector_draw_context, &scene_state.sectors,
		(byte*) scene_state.heightmap, (byte*) palace_texture_id_map, scene_state.map_size[0], scene_state.map_size[1]);

	scene_state.billboard_draw_context = init_billboard_draw_context(
		7,
		(Billboard) {2, {1.0f, 1.0f}, {4.5f, 0.5f, 6.5f}},
		(Billboard) {3, {1.0f, 1.0f}, {2.5f, 0.5f, 6.5f}},
		(Billboard) {4, {1.0f, 1.0f}, {3.0f, 3.5f, 11.5f}},
		(Billboard) {0, {1.0f, 1.0f}, {8.5f, 0.5f, 25.5f}},
		(Billboard) {1, {1.0f, 1.0f}, {5.0f, 0.5f, 22.5f}},
		(Billboard) {0, {1.0f, 1.0f}, {12.5f, 0.5f, 38.5f}},
		(Billboard) {28, {1.0f, 1.0f}, {21.5f, 0.5f, 24.5f}} // 40
	);

	scene_state.billboard_draw_context.texture_set = init_texture_set(
		TexNonRepeating, 2, 2, 64, 64,
		"../../../../assets/objects/teleporter.bmp",
		"../../../../assets/objects/health_kit.bmp",
		"../../../../assets/spritesheets/bogo.bmp", 2, 3, 6,
		"../../../../assets/spritesheets/trooper.bmp", 33, 1, 33
	);

	//////////

	scene_state.sector_draw_context.texture_set = init_texture_set(TexRepeating,
		// New + Checker:
		// 1, 0, 128, 128, "../../../../assets/walls/pyramid_bricks_4.bmp"

		// Palace:
		11, 0, 128, 128,
		"../../../../assets/walls/sand.bmp", "../../../../assets/walls/pyramid_bricks_4.bmp",
		"../../../../assets/walls/marble.bmp", "../../../../assets/walls/hieroglyph.bmp",
		"../../../../assets/walls/window.bmp", "../../../../assets/walls/saqqara.bmp",
		"../../../../assets/walls/sandstone.bmp", "../../../../assets/walls/cobblestone_3.bmp",
		"../../../../assets/walls/horses.bmp", "../../../../assets/walls/mesa.bmp",
		"../../../../assets/walls/arthouse_bricks.bmp"

		// Pyramid:
		/* 3, 0, 512, 512, "../../../../assets/walls/pyramid_bricks_4.bmp",
		"../../../../assets/walls/greece.bmp", "../../../../assets/walls/saqqara.bmp" */

		// Tiny:
		// 2, 0, 64, 64, "../../../../assets/walls/mesa.bmp", "../../../../assets/walls/hieroglyph.bmp"

		// Level 1:
		/* 8, 0, 256, 256, "../../../../assets/walls/sand.bmp",
		"../../../../assets/walls/cobblestone_2.bmp",
		"../../../../assets/walls/cobblestone_3.bmp",
		"../../../../assets/walls/stone_2.bmp",
		"../../../../assets/walls/pyramid_bricks_3.bmp",
		"../../../../assets/walls/hieroglyphics.bmp",
		"../../../../assets/walls/desert_snake.bmp",
		"../../../../assets/wolf/colorstone.bmp" */
		);

	enable_all_culling();
	glEnable(GL_MULTISAMPLE);

	SceneState* const scene_state_on_heap = malloc(sizeof(SceneState));
	*scene_state_on_heap = scene_state;
	sgl.any_data = scene_state_on_heap;

	return sgl;
}

void demo_17_drawer(const StateGL* const sgl) {
	const SceneState* const scene_state = (SceneState*) sgl -> any_data;

	static Camera camera;
	static PhysicsObject physics_obj;
	static byte first_call = 1;

	if (first_call) { // start new map: 1.5f, 0.5f, 1.5f
		init_camera(&camera, (vec3) {5.0f, 0.5f, 5.0f});
		physics_obj.heightmap = scene_state -> heightmap;
		physics_obj.map_size[0] = scene_state -> map_size[0];
		physics_obj.map_size[1] = scene_state -> map_size[1];
		first_call = 0;
	}

	update_camera(&camera, get_next_event(), &physics_obj);
	draw_visible_sectors(&scene_state -> sector_draw_context, &scene_state -> sectors, &camera);
	// Skybox after sectors b/c most skybox fragments would be unnecessarily drawn otherwise
	draw_skybox(scene_state -> skybox, &camera);
	draw_visible_billboards(&scene_state -> billboard_draw_context, &camera);
}

void demo_17_deinit(const StateGL* const sgl) {
	const SceneState* const scene_state = (SceneState*) sgl -> any_data;

	deinit_list(scene_state -> sectors);
	deinit_batch_draw_context(&scene_state -> sector_draw_context);
	deinit_batch_draw_context(&scene_state -> billboard_draw_context);

	deinit_skybox(scene_state -> skybox);
	free(sgl -> any_data);

	deinit_demo_vars(sgl);
}

#ifdef DEMO_17
int main(void) {
	make_application(demo_17_drawer, demo_17_init, demo_17_deinit);
}
#endif
