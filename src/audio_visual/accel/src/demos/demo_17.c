#include "../utils.c"
#include "../drawable_set.c"
#include "../skybox.c"
#include "../data/maps.c"

#include "../face.c"
#include "../camera.c"
#include "../culling.c"

/*
- NEXT: new_map
- NEXT 2: grab texture id maps from upper project to use here
- NEXT 3: a bounding volume hierarchy, maybe
- NEXT 4: composable billboard drawer, but before that, billboard batching + different billboard types

- Figure out if diffuse should depend on where player is - or ambient occlusion + simple dynamic lights
- Store the cpu index list in three-bit parts; bit 0 = vert or flat, bit 1 = ns or ew, and bit 2 = side
- A map maker. An init file that specifies textures and dimensions, draw/erase modes, export, and choose heights and textures
- More effeciently set statemap bit ranges
- For terrain, quite slow with 2/3 phong lighting + some objects popping out for half seconds
- camera tilt

- Read sprite crop from spritesheet
- Blit 2D sprite to whole screen
- Blit color rect to screen
- Good physics
- Then, flat weapon

- In the end, 5 shaders + accel components: sectors, billboards, skybox, weapon, ui elements
*/

typedef struct {
	DrawableSet sector_list;
	Skybox skybox;
} SceneState;

StateGL demo_17_init(void) {
	/*
	(byte*) palace_heightmap, (byte*) palace_texture_id_map, palace_width, palace_height
	(byte*) pyramid_heightmap, (byte*) pyramid_texture_id_map, pyramid_width, pyramid_height
	(byte*) tpt_heightmap, (byte*) tpt_texture_id_map, tpt_width, tpt_height
	(byte*) new_heightmap, (byte*) texture_id_map, new_width, new_height
	(byte*) terrain_heightmap, (byte*) texture_id_map, terrain_width, terrain_height
	(byte*) tiny_heightmap, (byte*) tiny_heightmap, tiny_width, tiny_height
	*/

	SceneState scene_state;

	StateGL sgl = {.vertex_array = init_vao(), .num_vertex_buffers = 0};
	List face_mesh_list;

	//////////
	scene_state.skybox = init_skybox("../assets/mountain_2.bmp");

	// static byte texture_id_map[terrain_height][terrain_width];
	init_face_mesh_and_sector_lists(&scene_state.sector_list, &face_mesh_list,
		(byte*) palace_heightmap, (byte*) palace_texture_id_map, palace_width, palace_height);

	init_sector_list_vbo_and_ibo(&scene_state.sector_list, &face_mesh_list);

	scene_state.sector_list.shader = init_shader_program(sector_vertex_shader, sector_fragment_shader);

	//////////
	sgl.num_textures = 0;
	scene_state.sector_list.texture_set = init_texture_set(TexRepeating, 128, 128,
		// New:
		// 1, "../../../../assets/walls/pyramid_bricks_4.bmp"

		// Palace:
		11, "../../../../assets/walls/sand.bmp", "../../../../assets/walls/pyramid_bricks_4.bmp",
		"../../../../assets/walls/marble.bmp", "../../../../assets/walls/hieroglyph.bmp",
		"../../../../assets/walls/window.bmp", "../../../../assets/walls/saqqara.bmp",
		"../../../../assets/walls/sandstone.bmp", "../../../../assets/walls/cobblestone_3.bmp",
		"../../../../assets/walls/horses.bmp", "../../../../assets/walls/mesa.bmp",
		"../../../../assets/walls/arthouse_bricks.bmp"

		// Pyramid:
		/* 3, "../../../../assets/walls/pyramid_bricks_4.bmp",
		"../../../../assets/walls/greece.bmp", "../../../../assets/walls/saqqara.bmp" */

		// Tiny:
		// 2, "../../../../assets/walls/mesa.bmp", "../../../../assets/walls/hieroglyph.bmp"
		);

	enable_all_culling();
	glEnable(GL_MULTISAMPLE);
	deinit_list(face_mesh_list);

	SceneState* const scene_state_on_heap = malloc(sizeof(SceneState));
	*scene_state_on_heap = scene_state;
	sgl.any_data = scene_state_on_heap;

	return sgl;
}

void demo_17_drawer(const StateGL* const sgl) {
	static Camera camera;
	static byte first_call = 1;

	if (first_call) { // start new map: 1.5f, 0.5f, 1.5f
		init_camera(&camera, (vec3) {5.0f, 5.0f, 5.0f}); // terrain: 34.5f, 13.50f, 25.2f
		first_call = 0;
	}

	update_camera(&camera, get_next_event());

	const SceneState* const scene_state = (SceneState*) sgl -> any_data;
	draw_sectors(&scene_state -> sector_list, &camera);
	draw_skybox(scene_state -> skybox, &camera);
}

void demo_17_deinit(const StateGL* const sgl) {
	const SceneState* const scene_state = (SceneState*) sgl -> any_data;
	deinit_drawable_set(&scene_state -> sector_list);
	deinit_skybox(scene_state -> skybox);
	free(sgl -> any_data);

	deinit_demo_vars(sgl);
}

#ifdef DEMO_17
int main(void) {
	make_application(demo_17_drawer, demo_17_init, demo_17_deinit);
}
#endif
