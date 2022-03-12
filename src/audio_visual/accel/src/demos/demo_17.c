#include "../utils.c"
#include "../skybox.c"
#include "../data/maps.c"

#include "../sector.c"
#include "../billboard.c"
#include "../camera.c"
#include "../event.c"
#include "../overlay.c"
#include "../shadow_map.c"

typedef struct {
	WeaponSprite weapon_sprite;

	BatchDrawContext sector_draw_context, billboard_draw_context;
	ShadowMapContext shadow_map_context;

	List sectors; // This is not in the sector draw context b/c the cpu list for that context consists of vertices
	const List billboard_animations, billboard_animation_instances;

	const Skybox skybox;
	byte* const heightmap;
	const byte map_size[2];
} SceneState;

StateGL demo_17_init(void) {
	StateGL sgl = {.vertex_array = init_vao(), .num_vertex_buffers = 0, .num_textures = 0};

	// 2 << 13 is the biggest size
	SceneState scene_state = {
		.shadow_map_context = init_shadow_map_context(2048, 4048,
			(vec3) {26.563328f, 31.701447f, 12.387274f},
			0.518362f, -1.225221f
		),

		// .weapon_sprite = init_weapon_sprite(0.5f, 0.07f, "../../../../assets/spritesheets/weapons/desecrator_cropped.bmp", 1, 8, 8),
		// .weapon_sprite = init_weapon_sprite(0.65f, 0.016f, "../../../../assets/spritesheets/weapons/whip.bmp", 4, 6, 22),
		.weapon_sprite = init_weapon_sprite(0.65f, 0.035f, "../../../../assets/spritesheets/weapons/snazzy_shotgun.bmp", 6, 10, 59),

		.billboard_animations = LIST_INITIALIZER(animation) (4,
			(Animation) {.texture_id_range = {2, 47}, .secs_per_frame = 0.02f}, // Flying carpet
			(Animation) {.texture_id_range = {48, 52}, .secs_per_frame = 0.15f}, // Torch
			(Animation) {.texture_id_range = {61, 63}, .secs_per_frame = 0.08f}, // Eddie, attacking
			(Animation) {.texture_id_range = {76, 79}, .secs_per_frame = 0.07f} // Trooper, idle
		),

		.billboard_animation_instances = LIST_INITIALIZER(billboard_animation_instance) (6,
			(BillboardAnimationInstance) {.ids = {.billboard = 4, .animation = 0}, .last_frame_time = 0.0f}, // Flying carpet
			(BillboardAnimationInstance) {.ids = {.billboard = 5, .animation = 1}, .last_frame_time = 0.0f}, // Torch

			(BillboardAnimationInstance) {.ids = {.billboard = 6, .animation = 2}, .last_frame_time = 0.0f}, // Eddies
			(BillboardAnimationInstance) {.ids = {.billboard = 7, .animation = 2}, .last_frame_time = 0.0f},

			(BillboardAnimationInstance) {.ids = {.billboard = 8, .animation = 3}, .last_frame_time = 0.0f}, // Troopers
			(BillboardAnimationInstance) {.ids = {.billboard = 9, .animation = 3}, .last_frame_time = 0.0f}
		),

		.skybox = init_skybox("../assets/desert.bmp"),
		.heightmap = (byte*) fortress_heightmap,
		.map_size = {fortress_width, fortress_height}
	};

	//////////
	// static byte texture_id_map[terrain_height][terrain_width];
	init_sector_draw_context(&scene_state.sector_draw_context, &scene_state.sectors,
		scene_state.heightmap, (byte*) fortress_texture_id_map, scene_state.map_size);

	scene_state.billboard_draw_context = init_billboard_draw_context(
		10,
		(Billboard) {0, {1.0f, 1.0f}, {28.0f, 2.5f, 31.0f}}, // Health kits
		(Billboard) {0, {1.0f, 1.0f}, {5.0f, 0.5f, 22.5f}},

		(Billboard) {1, {1.0f, 1.0f}, {12.5f, 0.5f, 38.5f}}, // Teleporters
		(Billboard) {1, {1.0f, 1.0f}, {8.5f, 0.5f, 25.5f}},

		(Billboard) {2, {1.0f, 1.0f}, {5.0f, 0.5f, 2.0f}}, // Flying carpet
		(Billboard) {48, {1.0f, 1.0f}, {7.5f, 0.5f, 12.5f}}, // Torch

		(Billboard) {61, {1.0f, 1.0f}, {6.5f, 0.5f, 21.5f}}, // Eddies
		(Billboard) {61, {1.0f, 1.0f}, {3.5f, 0.5f, 24.5f}},

		(Billboard) {76, {1.0f, 1.0f}, {3.0f, 1.5f, 9.5f}}, // Troopers
		(Billboard) {76, {1.0f, 1.0f}, {21.5f, 0.5f, 24.5f}}
	);

	scene_state.billboard_draw_context.texture_set = init_texture_set(
		TexNonRepeating, 2, 4, 128, 128,

		"../../../../assets/objects/health_kit.bmp",
		"../../../../assets/objects/teleporter.bmp",

		"../../../../assets/spritesheets/flying_carpet.bmp", 5, 10, 46,
		"../../../../assets/spritesheets/torch_2.bmp", 2, 3, 5,
		"../../../../assets/spritesheets/eddie.bmp", 23, 1, 23,
		"../../../../assets/spritesheets/trooper.bmp", 33, 1, 33
	);

	//////////

	scene_state.sector_draw_context.texture_set = init_texture_set(TexRepeating,
		// Fortress:
		4, 0, 256, 256,
		"../../../../assets/walls/viney_bricks.bmp",
		"../../../../assets/walls/marble.bmp",
		"../../../../assets/walls/vines.bmp",
		"../../../../assets/walls/stone_2.bmp"

		// Palace:
		/* 11, 0, 128, 128,
		"../../../../assets/walls/sand.bmp", "../../../../assets/walls/pyramid_bricks_4.bmp",
		"../../../../assets/walls/marble.bmp", "../../../../assets/walls/hieroglyph.bmp",
		"../../../../assets/walls/window.bmp", "../../../../assets/walls/saqqara.bmp",
		"../../../../assets/walls/sandstone.bmp", "../../../../assets/walls/cobblestone_3.bmp",
		"../../../../assets/walls/horses.bmp", "../../../../assets/walls/mesa.bmp",
		"../../../../assets/walls/arthouse_bricks.bmp" */

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

	render_all_sectors_to_shadow_map(&scene_state.shadow_map_context,
		&scene_state.sector_draw_context, get_next_event().screen_size, scene_state.map_size);

	sgl.any_data = malloc(sizeof(SceneState));
	memcpy(sgl.any_data, &scene_state, sizeof(SceneState));

	return sgl;
}

void demo_17_drawer(const StateGL* const sgl) {
	SceneState* const scene_state = (SceneState*) sgl -> any_data;
	const BatchDrawContext* const sector_draw_context = &scene_state -> sector_draw_context;
	ShadowMapContext* const shadow_map_context = &scene_state -> shadow_map_context;

	static Camera camera;
	static PhysicsObject physics_obj;
	static bool first_call = true;

	if (first_call) {
		init_camera(&camera, (vec3) {1.5f, 0.5f, 1.5f}); // {3.9f, 0.5f, 6.0f}, {12.5f, 3.5f, 22.5f}
		physics_obj.heightmap = scene_state -> heightmap;
		physics_obj.map_size[0] = scene_state -> map_size[0];
		physics_obj.map_size[1] = scene_state -> map_size[1];
		first_call = false;
	}

	const Event event = get_next_event();

	update_camera(&camera, event, &physics_obj);

	update_billboard_animation_instances(
		&scene_state -> billboard_animation_instances,
		&scene_state -> billboard_animations,
		&scene_state -> billboard_draw_context.buffers.cpu);

	if (keys[SDL_SCANCODE_C]) {
		memcpy(shadow_map_context -> light_context.pos, camera.pos, sizeof(vec3));
		memcpy(shadow_map_context -> light_context.dir, camera.dir, sizeof(vec3));
		render_all_sectors_to_shadow_map(shadow_map_context, sector_draw_context, event.screen_size, physics_obj.map_size);
	}

	// Skybox after sectors b/c most skybox fragments would be unnecessarily drawn otherwise
	draw_visible_sectors(sector_draw_context, shadow_map_context, &scene_state -> sectors, &camera);

	const Skybox* const skybox = &scene_state -> skybox;
	draw_skybox(*skybox, &camera);
	draw_visible_billboards(&scene_state -> billboard_draw_context, &camera);
	update_and_draw_weapon_sprite(&scene_state -> weapon_sprite, &camera, &event);
}

void demo_17_deinit(const StateGL* const sgl) {
	SceneState* const scene_state = (SceneState*) sgl -> any_data;

	deinit_weapon_sprite(&scene_state -> weapon_sprite);

	deinit_batch_draw_context(&scene_state -> sector_draw_context);
	deinit_batch_draw_context(&scene_state -> billboard_draw_context);

	deinit_shadow_map_context(&scene_state -> shadow_map_context);

	deinit_list(scene_state -> sectors);
	deinit_list(scene_state -> billboard_animations);
	deinit_list(scene_state -> billboard_animation_instances);

	deinit_skybox(scene_state -> skybox);

	free(scene_state);

	deinit_demo_vars(sgl);
}

#ifdef DEMO_17
int main(void) {
	make_application(demo_17_drawer, demo_17_init, demo_17_deinit);
}
#endif
