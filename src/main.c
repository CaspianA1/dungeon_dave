#ifndef MAIN_C
#define MAIN_C

#include "headers/main.h"
#include "headers/billboard.h"
#include "headers/maps.h"
#include "headers/sector.h"
#include "headers/normal_map_generation.h"

static void* main_init(void) {
	////////// Defining a bunch of level data

	const AnimationLayout billboard_animation_layouts[] = {
		{"../assets/spritesheets/flying_carpet.bmp", 5, 10, 46},
		{"../assets/spritesheets/torch_2.bmp", 2, 3, 5},
		{"../assets/spritesheets/eddie.bmp", 23, 1, 23},
		{"../assets/spritesheets/trooper.bmp", 33, 1, 33}
	};

	const Animation billboard_animations[] = {
		{.texture_id_range = {3, 48}, .secs_per_frame = 0.02f}, // Flying carpet
		{.texture_id_range = {49, 53}, .secs_per_frame = 0.15f}, // Torch
		{.texture_id_range = {62, 64}, .secs_per_frame = 0.08f}, // Eddie, attacking
		{.texture_id_range = {77, 80}, .secs_per_frame = 0.07f} // Trooper, idle
	};

	const BillboardAnimationInstance billboard_animation_instances[] = {
		{.ids = {.billboard = 7, .animation = 0}, .last_frame_time = 0.0f}, // Flying carpet
		{.ids = {.billboard = 8, .animation = 1}, .last_frame_time = 0.0f}, // Torch

		{.ids = {.billboard = 9, .animation = 2}, .last_frame_time = 0.0f}, // Eddies
		{.ids = {.billboard = 10, .animation = 2}, .last_frame_time = 0.0f},

		{.ids = {.billboard = 11, .animation = 3}, .last_frame_time = 0.0f}, // Troopers
		{.ids = {.billboard = 12, .animation = 3}, .last_frame_time = 0.0f}
	};

	const Billboard billboards[] = {
		{0, {1.0f, 1.0f}, {28.0f, 2.5f, 31.0f}}, // Health kits
		{0, {1.0f, 1.0f}, {5.0f, 0.5f, 22.5f}},

		{1, {1.0f, 1.0f}, {12.5f, 0.5f, 38.5f}}, // Teleporters
		{1, {1.0f, 1.0f}, {8.5f, 0.5f, 25.5f}},

		{2, {2.0f, 2.0f}, {4.5f, 4.0f, 12.5f}}, // Shabtis
		{2, {2.0f, 2.0f}, {10.5f, 1.0f, 25.0f}},
		{2, {2.0f, 2.0f}, {25.5f, 3.0f, 31.0f}},

		{3, {1.0f, 1.0f}, {5.0f, 0.5f, 2.0f}}, // Flying carpet
		{49, {1.0f, 1.0f}, {7.5f, 0.5f, 12.5f}}, // Torch

		{62, {1.0f, 1.0f}, {6.5f, 0.5f, 21.5f}}, // Eddies
		{62, {1.0f, 1.0f}, {3.5f, 0.5f, 24.5f}},

		{77, {1.0f, 1.0f}, {3.0f, 1.5f, 9.5f}}, // Troopers
		{77, {1.0f, 1.0f}, {21.5f, 0.5f, 24.5f}}
	};

	const GLchar *const still_billboard_texture_paths[] = {
		"../assets/objects/health_kit.bmp",
		"../assets/objects/teleporter.bmp",
		"../assets/objects/shabti.bmp"
	},

	*const still_face_textures[] = {
		// Palace:
		"../assets/walls/sand.bmp", "../assets/walls/pyramid_bricks_4.bmp",
		"../assets/walls/marble.bmp", "../assets/walls/hieroglyph.bmp",
		"../assets/walls/alkadhib.bmp", "../assets/walls/saqqara.bmp",
		"../assets/walls/sandstone.bmp", "../assets/walls/cobblestone_3.bmp",
		"../assets/walls/horses.bmp", "../assets/walls/mesa.bmp",
		"../assets/walls/arthouse_bricks.bmp"

		// Pyramid:
		/* "../assets/walls/pyramid_bricks_4.bmp",
		"../assets/walls/greece.bmp", "../assets/walls/saqqara.bmp" */

		// Fortress:
		/* "../assets/walls/viney_bricks.bmp", "../assets/walls/marble.bmp",
		"../assets/walls/vines.bmp", "../assets/walls/stone_2.bmp" */

		// Tiny:
		// "../assets/walls/mesa.bmp", "../assets/walls/hieroglyph.bmp"

		// Level 1:
		/* "../assets/walls/sand.bmp", "../assets/walls/cobblestone_2.bmp",
		"../assets/walls/cobblestone_3.bmp", "../assets/walls/stone_2.bmp",
		"../assets/walls/pyramid_bricks_3.bmp", "../assets/walls/hieroglyphics.bmp",
		"../assets/walls/desert_snake.bmp", "../assets/walls/colorstone.bmp" */

		// Architecture:
		/* "../assets/walls/sand.bmp",
		"../assets/walls/marble.bmp", "../assets/walls/gold.bmp",
		"../assets/walls/greece.bmp", "../assets/walls/pyramid_bricks_4.bmp" */
	};

	//////////

	SceneState scene_state = {
		.vao = init_vao(),

		.weapon_sprite = init_weapon_sprite(
			// 0.6f, 2.0f, 0.07f, (AnimationLayout) {"../assets/spritesheets/weapons/desecrator_cropped.bmp", 1, 8, 8}
			0.75f, 2.0f, 0.016f, (AnimationLayout) {"../assets/spritesheets/weapons/whip.bmp", 4, 6, 22}
			// 0.75f, 2.0f, 0.035f, (AnimationLayout) {"../assets/spritesheets/weapons/snazzy_shotgun.bmp", 6, 10, 59}
			// 0.8f, 1.0f, 0.04f, (AnimationLayout) {"../assets/spritesheets/weapons/reload_pistol.bmp", 4, 7, 28}
		),

		.billboard_animations = init_list(ARRAY_LENGTH(billboard_animations), Animation),
		.billboard_animation_instances = init_list(ARRAY_LENGTH(billboard_animation_instances), BillboardAnimationInstance),

		.skybox = init_skybox("../assets/skyboxes/desert.bmp", 1.0f),

		.heightmap = (const byte*) palace_heightmap,
		.texture_id_map = (const byte*) palace_texture_id_map,
		.map_size = {palace_width, palace_height}
	};

	push_array_to_list(&scene_state.billboard_animations,
		billboard_animations, ARRAY_LENGTH(billboard_animations));

	push_array_to_list(&scene_state.billboard_animation_instances,
		billboard_animation_instances, ARRAY_LENGTH(billboard_animation_instances));

	// static byte texture_id_map[terrain_height][terrain_width];
	init_sector_draw_context(&scene_state.sector_draw_context, &scene_state.sectors,
		scene_state.heightmap, scene_state.texture_id_map, scene_state.map_size[0], scene_state.map_size[1]);

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

	//////////

	init_camera(&scene_state.camera, (vec3) {1.5f, 0.5f, 1.5f}, scene_state.heightmap, scene_state.map_size);

	scene_state.shadow_map_context = init_shadow_map_context(4096, 4096,
		(vec3) {26.563328f, 31.701447f, 12.387274f}, 0.518362f, -1.225221f,
		scene_state.camera.far_clip_dist
	),

	//////////

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	void* const app_context = malloc(sizeof(SceneState));
	memcpy(app_context, &scene_state, sizeof(SceneState));
	return app_context;
}

static void main_drawer(void* const app_context) {
	SceneState* const scene_state = (SceneState*) app_context;
	const BatchDrawContext* const sector_draw_context = &scene_state -> sector_draw_context;
	ShadowMapContext* const shadow_map_context = &scene_state -> shadow_map_context;
	Camera* const camera = &scene_state -> camera;

	const Event event = get_next_event();
	update_camera(camera, event);

	update_billboard_animation_instances(
		&scene_state -> billboard_animation_instances,
		&scene_state -> billboard_animations,
		&scene_state -> billboard_draw_context.buffers.cpu);

	update_shadow_map(shadow_map_context, event.screen_size, draw_all_sectors_for_shadow_map, sector_draw_context);

	////////// The main drawing code

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	draw_visible_sectors(sector_draw_context, shadow_map_context,
		&scene_state -> sectors, camera, scene_state -> face_normal_map_set, event.screen_size);

	draw_visible_billboards(&scene_state -> billboard_draw_context, shadow_map_context, camera);

	/* Drawing the skybox after sectors and billboards because
	most skybox fragments would unnecessarily be drawn otherwise */
	draw_skybox(scene_state -> skybox, camera);
	update_and_draw_weapon_sprite(&scene_state -> weapon_sprite, camera, &event);
}

static void main_deinit(void* const app_context) {
	SceneState* const scene_state = (SceneState*) app_context;

	deinit_weapon_sprite(&scene_state -> weapon_sprite);

	deinit_batch_draw_context(&scene_state -> sector_draw_context);
	deinit_batch_draw_context(&scene_state -> billboard_draw_context);

	deinit_shadow_map_context(&scene_state -> shadow_map_context);

	deinit_list(scene_state -> sectors);
	deinit_list(scene_state -> billboard_animations);
	deinit_list(scene_state -> billboard_animation_instances);

	deinit_skybox(scene_state -> skybox);
	deinit_texture(scene_state -> face_normal_map_set);

	deinit_vao(scene_state -> vao);

	free(scene_state);
}

int main(void) {
	make_application(main_drawer, main_init, main_deinit);
}

#endif