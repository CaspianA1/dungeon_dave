#include "main.h"
#include "normal_map_generation.h"
#include "data/maps.h"
#include "utils/texture.h" // TODO: remove this when I don't need to contain the alpha test logic in this file anymore
#include "event.h"
#include "utils/alloc.h"

static void draw_all_objects_to_shadow_map(const CascadedShadowContext* const shadow_context,
	const SectorContext* const sector_context, const WeaponSprite* const weapon_sprite) {

	/* Curr problems with the weapon sprite shadow:
	- Partial transparency (i.e. translucency); should I handle that?

	- For binary transparency, how do I do it without aliasing?
	- Perhaps with a filtered cutout texture: https://www.cs.rpi.edu/~cutler/classes/advancedgraphics/S17/final_projects/anthony_philip.pdf
	- Since 1-bit textures don't exist in OpenGL, or in hardware, perhaps do a translucency map anyways (a separate texture map). It would be
		a bit hard to implement, but it would be worth it for the sake of billboards later too.
	- For a translucency map, I will need alpha blending, which requires ordered billboards, so I must sort my billboards front-to-back by their center
	- This will make alpha results for billboards better, and it will allow shadows for the weapon sprite that have less aliasing

	- Transparency code is sorta fragmented right now: alpha blending for the weapon,
		normally, alpha testing for transparent objects, and alpha to coverage for billboards

	- Integration with the depth shader is a bit messy; find a neat way to do that (this should go first in terms of priorities)
	- The weapon sprite's shadow is way too light since it's close to the ground; find a way to resolve that

	- A note: one billboard takes up 24 bytes, and 4 vec3s take up 48 bytes (and adding a 16-bit texture id makes that 64)
	- Perhaps define billboards + the weapon sprite through 1. center pos, 2. face normal, and 3. size?
	*/

	const GLuint depth_shader = shadow_context -> depth_shader;

	static GLint drawing_translucent_quads_id, frame_index_id;

	ON_FIRST_CALL(
		INIT_UNIFORM(drawing_translucent_quads, depth_shader);
		INIT_UNIFORM(frame_index, depth_shader);
		INIT_UNIFORM_VALUE(alpha_threshold, depth_shader, 1f, 0.2f);

		use_texture(weapon_sprite -> drawable.diffuse_texture, depth_shader, "alpha_test_sampler", TexSet, TU_WeaponSpriteDiffuse);
	);

	// Opaque objects are drawn first

	UPDATE_UNIFORM(drawing_translucent_quads, 1i, false);
	draw_all_sectors_to_shadow_context(sector_context);

	// Then, translucent objects after

	UPDATE_UNIFORM(drawing_translucent_quads, 1i, true);
	UPDATE_UNIFORM(frame_index, 1ui, weapon_sprite -> animation_context.curr_frame);
	draw_weapon_sprite_to_shadow_context(weapon_sprite);
}

static void main_drawer(void* const app_context, const Event* const event) {
	glClear(GL_DEPTH_BUFFER_BIT); // No color buffer clearing needed

	SceneContext* const scene_context = (SceneContext*) app_context;

	if (tick_title_screen(&scene_context -> title_screen, event)) return;

	////////// Some variable initialization

	const GLfloat curr_time_secs = event -> curr_time_secs;

	const SectorContext* const sector_context = &scene_context -> sector_context;
	const CascadedShadowContext* const shadow_context = &scene_context -> shadow_context;

	Camera* const camera = &scene_context -> camera;
	BillboardContext* const billboard_context = &scene_context -> billboard_context;
	WeaponSprite* const weapon_sprite = &scene_context -> weapon_sprite;
	const AmbientOcclusionMap ao_map = scene_context -> ao_map;

	////////// Scene updating

	update_camera(camera, *event, scene_context -> heightmap, scene_context -> map_size);
	update_billboards(billboard_context, curr_time_secs);
	update_weapon_sprite(weapon_sprite, camera, event);
	update_shadow_context(shadow_context, camera, event -> aspect_ratio);
	update_shared_shading_params(&scene_context -> shared_shading_params, camera, shadow_context);

	////////// Rendering to the shadow context

	enable_rendering_to_shadow_context(shadow_context);
	draw_all_objects_to_shadow_map(shadow_context, sector_context, weapon_sprite);
	disable_rendering_to_shadow_context(event -> screen_size);

	////////// The main drawing code

	const Skybox* const skybox = &scene_context -> skybox;
	draw_sectors(sector_context, shadow_context, skybox, camera -> frustum_planes, curr_time_secs, ao_map);

	// No backface culling or depth buffer writes for billboards, the skybox, or the weapon sprite
	WITHOUT_BINARY_RENDER_STATE(GL_CULL_FACE,
		WITH_RENDER_STATE(glDepthMask, GL_FALSE, GL_TRUE,
			draw_skybox(skybox); // Drawn before any translucent geometry

			WITH_BINARY_RENDER_STATE(GL_BLEND, // Blending for these two
				draw_billboards(billboard_context, shadow_context, skybox, camera, ao_map);
				draw_weapon_sprite(weapon_sprite, shadow_context, skybox, ao_map);
			);
		);
	);
}

static void* main_init(void) {
	////////// Defining a bunch of level data

	const GLchar *const still_billboard_texture_paths[] = {
		ASSET_PATH("objects/health_kit.bmp"),
		ASSET_PATH("objects/teleporter.bmp"),
		ASSET_PATH("objects/shabti.bmp")
	};

	const AnimationLayout billboard_animation_layouts[] = {
		{ASSET_PATH("spritesheets/flying_carpet.bmp"), 5, 10, 46},
		{ASSET_PATH("spritesheets/torch_2.bmp"), 2, 3, 5},
		{ASSET_PATH("spritesheets/eddie.bmp"), 23, 1, 23},
		{ASSET_PATH("spritesheets/trooper.bmp"), 33, 1, 33}
	};

	// TODO: make these texture id ranges relative to each animation layout
	const Animation billboard_animations[] = {
		{.texture_id_range = {3, 48}, .secs_for_frame = 0.02f}, // Flying carpet
		{.texture_id_range = {49, 53}, .secs_for_frame = 0.15f}, // Torch
		{.texture_id_range = {62, 64}, .secs_for_frame = 0.08f}, // Eddie, attacking
		{.texture_id_range = {77, 81}, .secs_for_frame = 0.07f}, // Trooper, idle
		{.texture_id_range = {82, 88}, .secs_for_frame = 0.07f}, // Trooper, chase
		{.texture_id_range = {89, 98}, .secs_for_frame = 0.07f}, // Trooper, attacking
	};

	const BillboardAnimationInstance billboard_animation_instances[] = {
		{.billboard_id = 10, .animation_id = 0}, // Flying carpet
		{.billboard_id = 11, .animation_id = 1}, // Torch

		{.billboard_id = 12, .animation_id = 2}, // Eddies
		{.billboard_id = 13, .animation_id = 2},

		{.billboard_id = 14, .animation_id = 3}, // Troopers
		{.billboard_id = 15, .animation_id = 4},
		{.billboard_id = 16, .animation_id = 5}
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

		{billboard_animations[0].texture_id_range.start, {1.0f, 1.0f}, {5.0f, 0.5f, 2.0f}}, // Flying carpet
		{billboard_animations[1].texture_id_range.start, {1.0f, 1.0f}, {7.5f, 0.5f, 12.5f}}, // Torch

		{billboard_animations[2].texture_id_range.start, {1.0f, 1.0f}, {6.5f, 0.5f, 21.5f}}, // Eddies
		{billboard_animations[2].texture_id_range.start, {1.0f, 1.0f}, {3.5f, 0.5f, 24.5f}},

		{billboard_animations[3].texture_id_range.start, {1.0f, 1.0f}, {3.0f, 1.5f, 9.5f}}, // Troopers
		{billboard_animations[4].texture_id_range.start, {1.0f, 1.0f}, {9.5f, 6.5f, 13.5f}},
		{billboard_animations[5].texture_id_range.start, {1.0f, 1.0f}, {21.5f, 0.5f, 24.5f}}
	};

	const GLchar* const still_face_texture_paths[] = {
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

	const NormalMapConfig
		sector_faces_normal_map_config = {.blur_radius = 1, .blur_std_dev = 0.1f, .intensity = 1.3f, .rescale_factor = 2.0f},
		billboards_normal_map_config = {.blur_radius = 0, .blur_std_dev = 0.0f, .intensity = 1.0f, .rescale_factor = 2.0f}, // This, with 2x scaling, uses about 100mb more memory
		weapon_normal_map_config = {.blur_radius = 1, .blur_std_dev = 0.4f, .intensity = 1.5f, .rescale_factor = 3.0f}; // TODO: vary this per weapon sprite, if needed

	//////////

	const byte
		// *const heightmap = (const byte*) blank_heightmap, *const texture_id_map = (const byte*) blank_heightmap, map_size[2] = {blank_width, blank_height};
		*const heightmap = (const byte*) palace_heightmap, *const texture_id_map = (const byte*) palace_texture_id_map, map_size[2] = {palace_width, palace_height};
		// *const heightmap = (const byte*) fortress_heightmap, *const texture_id_map = (const byte*) fortress_texture_id_map, map_size[2] = {fortress_width, fortress_height};
		// *const heightmap = (const byte*) level_one_heightmap, *const texture_id_map = (const byte*) level_one_texture_id_map, map_size[2] = {level_one_width, level_one_height};
		// *const heightmap = (const byte*) terrain_heightmap, *const texture_id_map = (const byte*) terrain_texture_id_map, map_size[2] = {terrain_width, terrain_height};

	const byte max_point_height = get_heightmap_max_point_height(heightmap, map_size);
	const GLfloat far_clip_dist = compute_world_far_clip_dist(map_size, max_point_height);

	const GLsizei num_cascades = 8; // 8 for palace, 16 for terrain
	specify_cascade_count_before_any_shader_compilation(num_cascades);

	// 1024 for palace, 1200 for terrain
	const struct {const GLsizei face, billboard, shadow_map;} texture_sizes = {128, 128, 1024};

	const vec3 init_pos = {0.5f, 0.0f, 0.5f}; // {1.5f, 0.5f, 1.5f};

	//////////

	const SceneContext scene_context = {
		.camera = init_camera(init_pos, far_clip_dist),

		.weapon_sprite = init_weapon_sprite(
			// 3.0f, 3.0f, 1.0f, 1.0f, &(AnimationLayout) {ASSET_PATH("walls/simple_squares.bmp"), 1, 1, 1}, &weapon_normal_map_config
			3.0f, 8.0f, 0.6f, 0.07f, &(AnimationLayout) {ASSET_PATH("spritesheets/weapons/desecrator_cropped.bmp"), 1, 8, 8}, &weapon_normal_map_config
			// 3.0f, 2.0f, 0.75f, 0.02f, &(AnimationLayout) {ASSET_PATH("spritesheets/weapons/whip.bmp"), 4, 6, 22}, &weapon_normal_map_config
			// 4.0f, 4.0f, 0.75f, 0.035f, &(AnimationLayout) {ASSET_PATH("spritesheets/weapons/snazzy_shotgun.bmp"), 6, 10, 59}, &weapon_normal_map_config
			// 2.0f, 2.0f, 0.8f, 0.04f, &(AnimationLayout) {ASSET_PATH("spritesheets/weapons/reload_pistol.bmp"), 4, 7, 28}, &weapon_normal_map_config
		),

		.sector_context = init_sector_context(heightmap, texture_id_map, map_size[0], map_size[1],
			still_face_texture_paths, ARRAY_LENGTH(still_face_texture_paths), texture_sizes.face, &sector_faces_normal_map_config
		),

		.billboard_context = init_billboard_context(
			texture_sizes.billboard, &billboards_normal_map_config,

			ARRAY_LENGTH(still_billboard_texture_paths), still_billboard_texture_paths,
			ARRAY_LENGTH(billboard_animation_layouts), billboard_animation_layouts,

			ARRAY_LENGTH(billboards), billboards,
			ARRAY_LENGTH(billboard_animations), billboard_animations,
			ARRAY_LENGTH(billboard_animation_instances), billboard_animation_instances
		),

		.shadow_context = init_shadow_context(
			// Terrain:
			/*
			(vec3) {0.241236f, 0.930481f, -0.275698f}, (vec3) {1.0f, 1.0f, 1.0f},
			far_clip_dist, 0.4f, texture_sizes.shadow_map, num_cascades
			*/

			// Palace:
			(vec3) {0.241236f, 0.930481f, -0.275698f}, (vec3) {1.0f, 1.75f, 1.0f},
			far_clip_dist, 0.25f, texture_sizes.shadow_map, num_cascades
		),

		.ao_map = init_ao_map(heightmap, map_size, max_point_height),

		.skybox = init_skybox(ASSET_PATH("skyboxes/desert.bmp")),
		.title_screen = init_title_screen(),

		.heightmap = heightmap, .map_size = {map_size[0], map_size[1]}
	};

	////////// Global state initialization

	/* This is for correct for when premultiplying alpha.
	See https://www.realtimerendering.com/blog/gpus-prefer-premultiplication/. */
	glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
	glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glDepthFunc(GL_LESS);

	/* Depth clamping is used for 1. shadow pancaking, 2. avoiding clipping with sectors when walking
	against them, and 3. stopping too much upwards weapon pitch from going through the near plane */
	const GLenum states[] = {GL_DEPTH_TEST, GL_DEPTH_CLAMP, GL_CULL_FACE, GL_TEXTURE_CUBE_MAP_SEAMLESS};
	for (byte i = 0; i < ARRAY_LENGTH(states); i++) glEnable(states[i]);

	////////// Initializing a scene context on the heap

	SceneContext* const scene_context_on_heap = alloc(1, sizeof(SceneContext));
	memcpy(scene_context_on_heap, &scene_context, sizeof(SceneContext));

	////////// Initializing shared shading params

	const GLuint shaders_that_use_shared_params[] = {
		scene_context.shadow_context.depth_shader,
		scene_context.skybox.shader,
		scene_context.sector_context.drawable.shader,
		scene_context.billboard_context.drawable.shader,
		scene_context.weapon_sprite.drawable.shader
	};

	const SharedShadingParams shared_shading_params = init_shared_shading_params(
		shaders_that_use_shared_params, ARRAY_LENGTH(shaders_that_use_shared_params),
		&scene_context.shadow_context
	);

	// I am bypassing the type system's const safety checks with this, but it's for the best
	memcpy(&scene_context_on_heap -> shared_shading_params, &shared_shading_params, sizeof(SharedShadingParams));

	//////////

	return scene_context_on_heap;
}

static void main_deinit(void* const app_context) {
	SceneContext* const scene_context = (SceneContext*) app_context;

	deinit_shared_shading_params(&scene_context -> shared_shading_params);

	deinit_weapon_sprite(&scene_context -> weapon_sprite);
	deinit_sector_context(&scene_context -> sector_context);
	deinit_billboard_context(&scene_context -> billboard_context);

	deinit_ao_map(scene_context -> ao_map);
	deinit_shadow_context(&scene_context -> shadow_context);
	deinit_title_screen(&scene_context -> title_screen);
	deinit_skybox(scene_context -> skybox);

	dealloc(scene_context);
}

int main(void) {
	make_application(main_drawer, main_init, main_deinit);
}
