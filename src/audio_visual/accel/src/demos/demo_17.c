#include "../utils.c"
#include "../skybox.c"
#include "../data/maps.c"
#include "../sector.c"
#include "../billboard.c"
#include "../camera.c"
#include "../event.c"
#include "../weapon_sprite.c"
#include "../animation.c"
#include "../shadow_map.c"
#include "../normal_map_generation.c"

typedef struct {
	WeaponSprite weapon_sprite;

	BatchDrawContext sector_draw_context, billboard_draw_context;
	ShadowMapContext shadow_map_context;
	VoxelPhysicsContext physics_context;

	List sectors, billboard_animations, billboard_animation_instances;

	const Skybox skybox;

	GLuint face_normal_map_set;

	const byte *const heightmap, *const texture_id_map, map_size[2];
} SceneState;

StateGL demo_17_init(void) {
	StateGL sgl = {.vertex_array = init_vao(), .num_vertex_buffers = 0, .num_textures = 0};

	////////// Defining a bunch of level data

	const AnimationLayout billboard_animation_layouts[] = {
		{"../../../../assets/spritesheets/flying_carpet.bmp", 5, 10, 46},
		{"../../../../assets/spritesheets/torch_2.bmp", 2, 3, 5},
		{"../../../../assets/spritesheets/eddie.bmp", 23, 1, 23},
		{"../../../../assets/spritesheets/trooper.bmp", 33, 1, 33}
	};

	const Animation billboard_animations[] = {
		{.texture_id_range = {3, 48}, .secs_per_frame = 0.02f}, // Flying carpet
		{.texture_id_range = {49, 53}, .secs_per_frame = 0.15f}, // Torch
		{.texture_id_range = {62, 64}, .secs_per_frame = 0.08f}, // Eddie, attacking
		{.texture_id_range = {77, 80}, .secs_per_frame = 0.07f} // Trooper, idle
	};

	const BillboardAnimationInstance billboard_animation_instances[] = {
		{.ids = {.billboard = 5, .animation = 0}, .last_frame_time = 0.0f}, // Flying carpet
		{.ids = {.billboard = 6, .animation = 1}, .last_frame_time = 0.0f}, // Torch

		{.ids = {.billboard = 7, .animation = 2}, .last_frame_time = 0.0f}, // Eddies
		{.ids = {.billboard = 8, .animation = 2}, .last_frame_time = 0.0f},

		{.ids = {.billboard = 9, .animation = 3}, .last_frame_time = 0.0f}, // Troopers
		{.ids = {.billboard = 10, .animation = 3}, .last_frame_time = 0.0f}
	};

	const Billboard billboards[] = {
		{0, {1.0f, 1.0f}, {28.0f, 2.5f, 31.0f}}, // Health kits
		{0, {1.0f, 1.0f}, {5.0f, 0.5f, 22.5f}},

		{1, {1.0f, 1.0f}, {12.5f, 0.5f, 38.5f}}, // Teleporters
		{1, {1.0f, 1.0f}, {8.5f, 0.5f, 25.5f}},

		{2, {2.0f, 2.0f}, {6.5f, 6.0f, 7.5f}}, // Shabti

		{3, {1.0f, 1.0f}, {5.0f, 0.5f, 2.0f}}, // Flying carpet
		{49, {1.0f, 1.0f}, {7.5f, 0.5f, 12.5f}}, // Torch

		{62, {1.0f, 1.0f}, {6.5f, 0.5f, 21.5f}}, // Eddies
		{62, {1.0f, 1.0f}, {3.5f, 0.5f, 24.5f}},

		{77, {1.0f, 1.0f}, {3.0f, 1.5f, 9.5f}}, // Troopers
		{77, {1.0f, 1.0f}, {21.5f, 0.5f, 24.5f}}
	};

	const GLchar *const still_billboard_texture_paths[] = {
		"../../../../assets/objects/health_kit.bmp",
		"../../../../assets/objects/teleporter.bmp",
		"../../../../assets/objects/shabti.bmp"
	},

	*const still_face_textures[] = {
		// Palace:
		"../../../../assets/walls/sand.bmp", "../../../../assets/walls/pyramid_bricks_4.bmp",
		"../../../../assets/walls/marble.bmp", "../../../../assets/walls/hieroglyph.bmp",
		"../../../../assets/walls/alkadhib.bmp", "../../../../assets/walls/saqqara.bmp",
		"../../../../assets/walls/sandstone.bmp", "../../../../assets/walls/cobblestone_3.bmp",
		"../../../../assets/walls/horses.bmp", "../../../../assets/walls/mesa.bmp",
		"../../../../assets/walls/arthouse_bricks.bmp"

		// Pyramid:
		/* "../../../../assets/walls/pyramid_bricks_4.bmp",
		"../../../../assets/walls/greece.bmp", "../../../../assets/walls/saqqara.bmp" */

		// Fortress:
		/* "../../../../assets/walls/viney_bricks.bmp", "../../../../assets/walls/marble.bmp",
		"../../../../assets/walls/vines.bmp", "../../../../assets/walls/stone_2.bmp" */

		// Tiny:
		// "../../../../assets/walls/mesa.bmp", "../../../../assets/walls/hieroglyph.bmp"

		// Level 1:
		/* "../../../../assets/walls/sand.bmp", "../../../../assets/walls/cobblestone_2.bmp",
		"../../../../assets/walls/cobblestone_3.bmp", "../../../../assets/walls/stone_2.bmp",
		"../../../../assets/walls/pyramid_bricks_3.bmp", "../../../../assets/walls/hieroglyphics.bmp",
		"../../../../assets/walls/desert_snake.bmp", "../../../../assets/wolf/colorstone.bmp" */

		// Architecture:
		/* "../../../../assets/walls/sand.bmp",
		"../../../../assets/walls/marble.bmp", "../../../../assets/walls/gold.bmp",
		"../../../../assets/walls/greece.bmp", "../../../../assets/walls/pyramid_bricks_4.bmp" */
	};

	//////////

	/* For a 2048x2048 shadow map:
	- One texture = 2048 * 2048 * 16 = 67,108,864 bytes
	- One texture is mipmapped, so one takes up 89,478,486 bytes
	- Total is 156,587,350 bytes */

	SceneState scene_state = { // 2 << 13 is the biggest size
		.shadow_map_context = init_shadow_map_context(2048, 2048,
			// (vec3) {40.0f, 15.0f, 0.0f}, 5.5f, -1.0f
			(vec3) {26.563328f, 31.701447f, 12.387274f}, 0.518362f, -1.225221f
		),

		.weapon_sprite = init_weapon_sprite(
			// 0.6f, 2.0f, 0.07f, (AnimationLayout) {"../../../../assets/spritesheets/weapons/desecrator_cropped.bmp", 1, 8, 8}
			0.75f, 2.0f, 0.016f, (AnimationLayout) {"../../../../assets/spritesheets/weapons/whip.bmp", 4, 6, 22}
			// 0.75f, 2.0f, 0.035f, (AnimationLayout) {"../../../../assets/spritesheets/weapons/snazzy_shotgun.bmp", 6, 10, 59}
		),

		.billboard_animations = init_list(ARRAY_LENGTH(billboard_animations), Animation),
		.billboard_animation_instances = init_list(ARRAY_LENGTH(billboard_animation_instances), BillboardAnimationInstance),

		.skybox = init_skybox("../assets/desert.bmp"),

		.heightmap = (const byte*) palace_heightmap,
		.texture_id_map = (const byte*) palace_texture_id_map,
		.map_size = {palace_width, palace_height}
	};

	push_array_to_list(&scene_state.billboard_animations,
		billboard_animations, ARRAY_LENGTH(billboard_animations));

	push_array_to_list(&scene_state.billboard_animation_instances,
		billboard_animation_instances, ARRAY_LENGTH(billboard_animation_instances));

	//////////

	const byte scene_map_width = scene_state.map_size[0], scene_map_height = scene_state.map_size[1];

	scene_state.physics_context = init_physics_context(scene_state.heightmap, scene_map_width, scene_map_height);

	// static byte texture_id_map[terrain_height][terrain_width];
	init_sector_draw_context(&scene_state.sector_draw_context, &scene_state.sectors,
		scene_state.heightmap, scene_state.texture_id_map, scene_map_width, scene_map_height);

	scene_state.billboard_draw_context = init_billboard_draw_context(ARRAY_LENGTH(billboards), billboards);

	scene_state.billboard_draw_context.texture_set = init_texture_set(
		TexNonRepeating, OPENGL_SCENE_MAG_FILTER, OPENGL_SCENE_MIN_FILTER,
		ARRAY_LENGTH(still_billboard_texture_paths), ARRAY_LENGTH(billboard_animation_layouts), 256, 256,
		still_billboard_texture_paths, billboard_animation_layouts
	);

	//////////

	scene_state.sector_draw_context.texture_set = init_texture_set(
		TexRepeating, OPENGL_SCENE_MAG_FILTER, OPENGL_SCENE_MIN_FILTER,
		ARRAY_LENGTH(still_face_textures), 0, 256, 256, still_face_textures, NULL
	);

	scene_state.face_normal_map_set = init_normal_map_set_from_texture_set(scene_state.sector_draw_context.texture_set, true);

	enable_all_culling();

	render_all_sectors_to_shadow_map(&scene_state.shadow_map_context,
		&scene_state.sector_draw_context, get_next_event().screen_size,
		scene_state.physics_context.far_clip_dist);

	sgl.any_data = malloc(sizeof(SceneState));
	memcpy(sgl.any_data, &scene_state, sizeof(SceneState));

	return sgl;
}

void demo_17_drawer(const StateGL* const sgl) {
	SceneState* const scene_state = (SceneState*) sgl -> any_data;
	const BatchDrawContext* const sector_draw_context = &scene_state -> sector_draw_context;
	ShadowMapContext* const shadow_map_context = &scene_state -> shadow_map_context;
	VoxelPhysicsContext* const physics_context = &scene_state -> physics_context;

	static Camera camera;
	static bool first_call = true;

	if (first_call) {
		init_camera(&camera, (vec3) {1.5f, 0.5f, 1.5f}); // {3.9f, 0.5f, 6.0f}, {12.5f, 3.5f, 22.5f}
		first_call = false;
	}

	const Event event = get_next_event();
	update_camera(&camera, event, physics_context);

	update_billboard_animation_instances(
		&scene_state -> billboard_animation_instances,
		&scene_state -> billboard_animations,
		&scene_state -> billboard_draw_context.buffers.cpu);

	if (keys[SDL_SCANCODE_C]) {
		glm_vec3_copy(camera.pos, shadow_map_context -> light_context.pos);
		glm_vec3_copy(camera.dir, shadow_map_context -> light_context.dir);
		render_all_sectors_to_shadow_map(shadow_map_context, sector_draw_context,
			event.screen_size, physics_context -> far_clip_dist);
	}

	// Skybox after sectors b/c most skybox fragments would be unnecessarily drawn otherwise
	draw_visible_sectors(sector_draw_context, shadow_map_context,
		&scene_state -> sectors, &camera, scene_state -> face_normal_map_set, event.screen_size);

	draw_skybox(scene_state -> skybox, &camera);
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
	deinit_texture(scene_state -> face_normal_map_set);

	free(scene_state);

	deinit_demo_vars(sgl);
}

#ifdef DEMO_17
int main(void) {
	make_application(demo_17_drawer, demo_17_init, demo_17_deinit);
}
#endif
