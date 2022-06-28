#ifndef MAIN_C
#define MAIN_C

#include "headers/main.h"
#include "headers/maps.h"
#include "headers/texture.h"

static void* main_init(void) {
	////////// Defining a bunch of level data

	const AnimationLayout billboard_animation_layouts[] = {
		{ASSET_PATH("spritesheets/flying_carpet.bmp"), 5, 10, 46},
		{ASSET_PATH("spritesheets/torch_2.bmp"), 2, 3, 5},
		{ASSET_PATH("spritesheets/eddie.bmp"), 23, 1, 23},
		{ASSET_PATH("spritesheets/trooper.bmp"), 33, 1, 33}
	};

	const Animation billboard_animations[] = {
		{.texture_id_range = {3, 48}, .secs_for_frame = 0.02f}, // Flying carpet
		{.texture_id_range = {49, 53}, .secs_for_frame = 0.15f}, // Torch
		{.texture_id_range = {62, 64}, .secs_for_frame = 0.08f}, // Eddie, attacking
		{.texture_id_range = {77, 80}, .secs_for_frame = 0.07f} // Trooper, idle
	};

	const BillboardAnimationInstance billboard_animation_instances[] = {
		{.ids = {.billboard = 10, .animation = 0}}, // Flying carpet
		{.ids = {.billboard = 11, .animation = 1}}, // Torch

		{.ids = {.billboard = 12, .animation = 2}}, // Eddies
		{.ids = {.billboard = 13, .animation = 2}},

		{.ids = {.billboard = 14, .animation = 3}}, // Troopers
		{.ids = {.billboard = 15, .animation = 3}}
	};

	const Billboard billboards[] = {
		{0, {1.0f, 1.0f}, {28.0f, 2.5f, 31.0f}}, // Health kits
		{0, {1.0f, 1.0f}, {5.0f, 0.5f, 22.5f}},
		{0, {1.0f, 1.0f}, {31.5f, 0.5f, 10.5f}},

		{1, {1.0f, 1.0f}, {12.5f, 0.5f, 38.5f}}, // Teleporters
		{1, {1.0f, 1.0f}, {8.5f, 0.5f, 25.5f}},
		{1, {1.0f, 1.0f}, {32.5f, 2.5f, 7.5f}},

		{2, {2.0f, 2.0f}, {4.5f, 4.0f, 12.5f}}, // Shabtis
		{2, {2.0f, 2.0f}, {10.5f, 1.0f, 25.0f}},
		{2, {2.0f, 2.0f}, {25.5f, 3.0f, 31.0f}},
		{2, {4.0f, 4.0f}, {36.0f, 18.0f, 4.0f}},

		{3, {1.0f, 1.0f}, {5.0f, 0.5f, 2.0f}}, // Flying carpet
		{49, {1.0f, 1.0f}, {7.5f, 0.5f, 12.5f}}, // Torch

		{62, {1.0f, 1.0f}, {6.5f, 0.5f, 21.5f}}, // Eddies
		{62, {1.0f, 1.0f}, {3.5f, 0.5f, 24.5f}},

		{77, {1.0f, 1.0f}, {3.0f, 1.5f, 9.5f}}, // Troopers
		{77, {1.0f, 1.0f}, {21.5f, 0.5f, 24.5f}}
	};

	const GLchar *const still_billboard_texture_paths[] = {
		ASSET_PATH("objects/health_kit.bmp"),
		ASSET_PATH("objects/teleporter.bmp"),
		ASSET_PATH("objects/shabti.bmp")
	},

	*const still_face_textures[] = {
		// Palace:
		ASSET_PATH("walls/sand.bmp"), ASSET_PATH("walls/pyramid_bricks_4.bmp"),
		ASSET_PATH("walls/marble.bmp"), ASSET_PATH("walls/hieroglyph.bmp"),
		ASSET_PATH("walls/alkadhib.bmp"), ASSET_PATH("walls/saqqara.bmp"),
		ASSET_PATH("walls/sandstone.bmp"), ASSET_PATH("walls/cobblestone_3.bmp"),
		ASSET_PATH("walls/rug_3.bmp"), ASSET_PATH("walls/mesa.bmp"),
		ASSET_PATH("walls/arthouse_bricks.bmp"), ASSET_PATH("walls/eye_of_evil.bmp"),
		ASSET_PATH("walls/rough_marble.bmp"), ASSET_PATH("walls/mosaic.bmp"),
		ASSET_PATH("walls/aquamarine_tiles.bmp")

		// Pyramid:
		/*
		ASSET_PATH("walls/pyramid_bricks_4.bmp"),
		ASSET_PATH("walls/greece.bmp"), ASSET_PATH("walls/saqqara.bmp")
		*/

		// Fortress:
		/*
		ASSET_PATH("walls/viney_bricks.bmp"), ASSET_PATH("walls/marble.bmp"),
		ASSET_PATH("walls/vines.bmp"),  ASSET_PATH("walls/stone_2.bmp")
		*/

		// Tiny:
		// ASSET_PATH("walls/mesa.bmp"), ASSET_PATH("walls/hieroglyph.bmp")

		// Level 1:
		/*
		ASSET_PATH("walls/sand.bmp"), ASSET_PATH("walls/cobblestone_2.bmp"),
		ASSET_PATH("walls/cobblestone_3.bmp"), ASSET_PATH("walls/stone_2.bmp"),
		ASSET_PATH("walls/pyramid_bricks_3.bmp"), ASSET_PATH("walls/hieroglyphics.bmp"),
		ASSET_PATH("walls/desert_snake.bmp"), ASSET_PATH("walls/colorstone.bmp")
		*/

		// Architecture:
		/*
		ASSET_PATH("walls/sand.bmp"),
		ASSET_PATH("walls/marble.bmp"), ASSET_PATH("walls/gold.bmp"),
		ASSET_PATH("walls/greece.bmp"), ASSET_PATH("walls/pyramid_bricks_4.bmp")
		*/
	};

	//////////

	const byte
		*const heightmap = (const byte*) palace_heightmap,
		*const texture_id_map = (const byte*) palace_texture_id_map,
		map_size[2] = {palace_width, palace_height};

	const GLfloat far_clip_dist = compute_world_far_clip_dist(heightmap, map_size);

	//////////

	SceneContext scene_context = {
		.camera = init_camera((vec3) {1.5f, 0.5f, 1.5f}, far_clip_dist),

		.weapon_sprite = init_weapon_sprite(
			// 0.6f, 1.0f, 1.0f, 1.0f, 1.0f, (AnimationLayout) {ASSET_PATH("walls/simple_squares.bmp"), 1, 1, 1}
			0.5f, 0.5f, 0.6f, 2.0f, 0.07f, (AnimationLayout) {ASSET_PATH("spritesheets/weapons/desecrator_cropped.bmp"), 1, 8, 8}
			// 0.4f, 0.5f, 0.75f, 2.0f, 0.02f, (AnimationLayout) {ASSET_PATH("spritesheets/weapons/whip.bmp"), 4, 6, 22}
			// 0.5f, 0.2f, 0.75f, 2.0f, 0.035f, (AnimationLayout) {ASSET_PATH("spritesheets/weapons/snazzy_shotgun.bmp"), 6, 10, 59}
			// 0.5f, 0.3f, 0.8f, 1.0f, 0.04f, (AnimationLayout) {ASSET_PATH("spritesheets/weapons/reload_pistol.bmp"), 4, 7, 28}
		),

		.sector_context = init_sector_context(heightmap, texture_id_map, map_size[0], map_size[1],
			true, init_texture_set(TexRepeating, OPENGL_SCENE_MAG_FILTER, OPENGL_SCENE_MIN_FILTER,
				ARRAY_LENGTH(still_face_textures), 0, 256, 256, still_face_textures, NULL
			)
		),

		.billboard_context = init_billboard_context(
			init_texture_set(
				TexNonRepeating, OPENGL_SCENE_MAG_FILTER, OPENGL_SCENE_MIN_FILTER,
				ARRAY_LENGTH(still_billboard_texture_paths), ARRAY_LENGTH(billboard_animation_layouts),
				256, 256, still_billboard_texture_paths, billboard_animation_layouts
			),

			ARRAY_LENGTH(billboards), billboards,
			ARRAY_LENGTH(billboard_animations), billboard_animations,
			ARRAY_LENGTH(billboard_animation_instances), billboard_animation_instances
		),

		.cascaded_shadow_context = init_shadow_context(
			// Terrain:
			/*
			(vec3) {0.241236f, 0.930481f, -0.275698f}, (vec3) {1.5f, 1.5f, 15.0f},
			far_clip_dist, 0.3f, 1024, 1024, 6
			*/

			// Palace:
			(vec3) {0.241236f, 0.930481f, -0.275698f}, (vec3) {1.5f, 1.5f, 5.0f},
			far_clip_dist, 0.4f, 1024, 1024, 8
		),

		.skybox = init_skybox(ASSET_PATH("skyboxes/desert.bmp"), 1.0f),
		.title_screen = init_title_screen(ASSET_PATH("logo.bmp")),

		.heightmap = heightmap, .map_size = {map_size[0], map_size[1]}
	};

	glDepthFunc(GL_LESS);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	const GLenum states[] = {GL_DEPTH_TEST, GL_CULL_FACE, GL_TEXTURE_CUBE_MAP_SEAMLESS};
	for (byte i = 0; i < ARRAY_LENGTH(states); i++) glEnable(states[i]);

	void* const app_context = malloc(sizeof(SceneContext));
	memcpy(app_context, &scene_context, sizeof(SceneContext));
	return app_context;
}

static void main_drawer(void* const app_context) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	SceneContext* const scene_context = (SceneContext*) app_context;
	const Event event = get_next_event();

	if (tick_title_screen(&scene_context -> title_screen, &event)) return;

	////////// Some variable initialization + object updating

	const SectorContext* const sector_context = &scene_context -> sector_context;
	const BillboardContext* const billboard_context = &scene_context -> billboard_context;
	const CascadedShadowContext* const shadow_context = &scene_context -> cascaded_shadow_context;
	Camera* const camera = &scene_context -> camera;

	update_camera(camera, event, scene_context -> heightmap, scene_context -> map_size);

	//////////

	update_billboards(billboard_context);
	draw_to_shadow_context(shadow_context, camera, event.screen_size, draw_all_sectors_for_shadow_map, &sector_context -> draw_context);

	////////// The main drawing code

	draw_visible_sectors(sector_context, shadow_context, camera, event.screen_size);
	draw_visible_billboards(billboard_context, shadow_context, camera);

	/* Drawing the skybox after sectors and billboards because
	most skybox fragments would unnecessarily be drawn otherwise */
	draw_skybox(&scene_context -> skybox, camera -> view_projection);

	update_and_draw_weapon_sprite(&scene_context -> weapon_sprite, camera, &event, shadow_context);
}

static void main_deinit(void* const app_context) {
	SceneContext* const scene_context = (SceneContext*) app_context;

	deinit_weapon_sprite(&scene_context -> weapon_sprite);
	deinit_sector_context(&scene_context -> sector_context);
	deinit_billboard_context(&scene_context -> billboard_context);
	deinit_shadow_context(&scene_context -> cascaded_shadow_context);
	deinit_title_screen(&scene_context -> title_screen);
	deinit_skybox(scene_context -> skybox);

	free(scene_context);
}

int main(void) {
	make_application(main_drawer, main_init, main_deinit);
}

#endif
