#include "main.h"
#include "utils/opengl_wrappers.h" // For OpenGL defs + wrappers
#include "utils/macro_utils.h" // For `ASSET_PATH`, and `ARRAY_LENGTH`
#include "data/constants.h" // For `num_unique_object_types`
#include "data/maps.h" // For various heightmaps and texture id maps
#include "utils/map_utils.h" // For `get_heightmap_max_point_height` and `compute_world_far_clip_dist`
#include "utils/alloc.h" // For `alloc`, and `dealloc`, and `WindowConfig`
#include "window.h" // For `make_application`
#include "utils/debug_macro_utils.h" // For the debug keys, and `DEBUG_VEC3`
#include "utils/json.h" // For various json defs

static bool main_drawer(void* const app_context, const Event* const event) {
	////////// Setting the wireframe mode

	const Uint8* const keys = event -> keys;

	static bool in_wireframe_mode = false, already_pressing_wireframe_mode_key = false;

	if (keys[KEY_TOGGLE_WIREFRAME_MODE]) {
		if (!already_pressing_wireframe_mode_key) {
			already_pressing_wireframe_mode_key = true;
			glPolygonMode(GL_FRONT_AND_BACK, (in_wireframe_mode = !in_wireframe_mode) ? GL_LINE : GL_FILL);
		}
	}
	else already_pressing_wireframe_mode_key = false;

	glClear(GL_DEPTH_BUFFER_BIT | (in_wireframe_mode * GL_COLOR_BUFFER_BIT));

	//////////

	SceneContext* const scene_context = (SceneContext*) app_context;
	if (tick_title_screen(&scene_context -> title_screen, event)) return true;

	////////// Some variable initialization

	const SectorContext* const sector_context = &scene_context -> sector_context;
	const CascadedShadowContext* const shadow_context = &scene_context -> shadow_context;

	Camera* const camera = &scene_context -> camera;
	BillboardContext* const billboard_context = &scene_context -> billboard_context;
	WeaponSprite* const weapon_sprite = &scene_context -> weapon_sprite;
	const AudioContext* const audio_context = &scene_context -> audio_context;

	DynamicLight* const dynamic_light = &scene_context -> dynamic_light;
	const GLfloat* const dir_to_light = dynamic_light -> curr_dir, curr_time_secs = event -> curr_time_secs;

	////////// Scene updating

	update_camera(camera, event, scene_context -> heightmap, scene_context -> map_size);

	update_billboard_context(billboard_context, curr_time_secs);
	update_weapon_sprite(weapon_sprite, camera, event);
	update_dynamic_light(dynamic_light, curr_time_secs);
	update_shadow_context(shadow_context, camera, dir_to_light, event -> aspect_ratio);
	update_shared_shading_params(&scene_context -> shared_shading_params, camera, shadow_context, dir_to_light);
	update_audio_context(audio_context, camera);

	////////// Rendering to the shadow context

	// TODO: still enable face culling for sectors?
	WITHOUT_BINARY_RENDER_STATE(GL_CULL_FACE,
		enable_rendering_to_shadow_context(shadow_context);
			draw_sectors_to_shadow_context(sector_context);
			draw_billboards_to_shadow_context(billboard_context);
		disable_rendering_to_shadow_context(event -> screen_size);
	);

	////////// The main drawing code

	draw_sectors(sector_context, camera);

	// No backface culling or depth buffer writes for the skybox, billboards, or the weapon sprite
	WITHOUT_BINARY_RENDER_STATE(GL_CULL_FACE,
		WITH_RENDER_STATE(glDepthMask, GL_FALSE, GL_TRUE,
			draw_skybox(&scene_context -> skybox); // Drawn before any translucent geometry

			WITH_BINARY_RENDER_STATE(GL_BLEND, // Blending for these two
				draw_billboards(billboard_context, camera);
				draw_weapon_sprite(weapon_sprite);
			);
		);
	);

	////////// Some debugging

	if (keys[KEY_PRINT_POSITION]) DEBUG_VEC3(camera -> pos);
	if (keys[KEY_PRINT_DIRECTION]) DEBUG_VEC3(camera -> dir);

	if (keys[KEY_PRINT_SDL_ERROR]) SDL_ERR_CHECK;
	if (keys[KEY_PRINT_OPENGL_ERROR]) GL_ERR_CHECK;
	if (keys[KEY_PRINT_AL_ERROR]) AL_ERR_CHECK;
	if (keys[KEY_PRINT_ALC_ERROR]) ALC_ERR_CHECK;

	return false;
}

static void* main_init(const WindowConfig* const window_config) {
	////////// Printing library info

	AudioContext audio_context = init_audio_context();

	#define PRINT_LIBRARY_INFO(suffix_lowercase, suffix_uppercase, start, end)\
		printf("\n%s Open%s:\nVendor: %s\nRenderer: %s\nVersion: %s\n%s",\
			start,\
			#suffix_uppercase,\
			suffix_lowercase##GetString(suffix_uppercase##_VENDOR),\
			suffix_lowercase##GetString(suffix_uppercase##_RENDERER),\
			suffix_lowercase##GetString(suffix_uppercase##_VERSION),\
			end)

	PRINT_LIBRARY_INFO(gl, GL, "---", "");
	PRINT_LIBRARY_INFO(al, AL, "---", "---\n\n");

	#undef PRINT_LIBRARY_INFO

	////////// Defining a bunch of level data

	cJSON* const level_json = init_json_from_file(ASSET_PATH("json_data/levels/palace.json"));

	cJSON // TODO: genericize this naming thing here via a macro
		*const parallax_json = read_json_subobj(level_json, "parallax_mapping"),
		*const shadow_mapping_json = read_json_subobj(level_json, "shadow_mapping"),
		*const vol_lighting_json =  read_json_subobj(level_json, "volumetric_lighting"),
		*const ao_json = read_json_subobj(level_json, "ambient_occlusion"),
		*const dyn_light_json = read_json_subobj(level_json, "dynamic_light"),
		*const skybox_json = read_json_subobj(level_json, "skybox");

	cJSON
		*const dyn_light_looking_at_json = read_json_subobj(dyn_light_json, "looking_at"),
		*const cascaded_shadow_json = read_json_subobj(shadow_mapping_json, "cascades");

	vec3 dyn_light_pos, dyn_light_looking_at_origin, dyn_light_looking_at_dest;
	sdl_pixel_component_t rgb_light_color[3];

	GET_ARRAY_VALUES_FROM_JSON_KEY(dyn_light_json, dyn_light_pos, pos, float);
	GET_ARRAY_VALUES_FROM_JSON_KEY(dyn_light_looking_at_json, dyn_light_looking_at_origin, origin, float);
	GET_ARRAY_VALUES_FROM_JSON_KEY(dyn_light_looking_at_json, dyn_light_looking_at_dest, dest, float);
	GET_ARRAY_VALUES_FROM_JSON_KEY(level_json, rgb_light_color, rgb_light_color, u8);

	//////////

	const LevelRenderingConfig level_rendering_config = {
		// TODO: put more level rendering params in here

		.parallax_mapping = {
			JSON_TO_FIELD(parallax_json, enabled, bool),
			JSON_TO_FIELD(parallax_json, min_layers, float),
			JSON_TO_FIELD(parallax_json, max_layers, float),
			JSON_TO_FIELD(parallax_json, height_scale, float),
			JSON_TO_FIELD(parallax_json, lod_cutoff, float)
		},

		.shadow_mapping = {
			JSON_TO_FIELD(shadow_mapping_json, sample_radius, u8),
			JSON_TO_FIELD(shadow_mapping_json, esm_exponent, u8),

			JSON_TO_FIELD(shadow_mapping_json, esm_exponent_layer_scale_factor, float),
			JSON_TO_FIELD(shadow_mapping_json, billboard_alpha_threshold, float),

			.cascaded_shadow_config = {
				JSON_TO_FIELD(cascaded_shadow_json, num_cascades, u8),
				JSON_TO_FIELD(cascaded_shadow_json, num_depth_buffer_bits, u8),
				JSON_TO_FIELD(cascaded_shadow_json, resolution, u16),
				JSON_TO_FIELD(cascaded_shadow_json, sub_frustum_scale, float),
				JSON_TO_FIELD(cascaded_shadow_json, linear_split_weight, float)

				// Terrain
				/*
				.num_cascades = 16, .num_depth_buffer_bits = 16,
				.resolution = 1200, .sub_frustum_scale = 1.0f, .linear_split_weight = 0.4f
				*/
			}
		},

		.volumetric_lighting = {
			JSON_TO_FIELD(vol_lighting_json, num_samples, u8),
			JSON_TO_FIELD(vol_lighting_json, sample_density, float),
			JSON_TO_FIELD(vol_lighting_json, opacity, float)
		},

		.ambient_occlusion = {
			JSON_TO_FIELD(ao_json, tricubic_filtering_enabled, bool),
			JSON_TO_FIELD(ao_json, strength, float)
		},

		// Palace
		.dynamic_light_config = {
			JSON_TO_FIELD(dyn_light_json, time_for_cycle, float),

			.pos = {dyn_light_pos[0], dyn_light_pos[1], dyn_light_pos[2]},

			.looking_at = {
				.origin = {
					dyn_light_looking_at_origin[0],
					dyn_light_looking_at_origin[1],
					dyn_light_looking_at_origin[2]
				},

				.dest = {
					dyn_light_looking_at_dest[0],
					dyn_light_looking_at_dest[1],
					dyn_light_looking_at_dest[2]
				}
			}
		},

		.skybox_config = {
			JSON_TO_FIELD(skybox_json, texture_path, string),
			JSON_TO_FIELD(skybox_json, map_cube_to_sphere, bool)
		},

		// Fortress
		/*
		.dynamic_light_config = {
			.time_for_cycle = 1.0f,
			.pos = {22.0f, 17.0f, 31.0f},

			.looking_at = {
				.origin = {2.0f, 0.0f, 1.0f},
				.dest = {2.0f, 0.0f, 1.0f}
			}
		},

		.skybox_config = {
			.texture_path = ASSET_PATH("skyboxes/sky_3.bmp"),
			.map_cube_to_sphere = false
		},
		*/

		.rgb_light_color = {rgb_light_color[0], rgb_light_color[1], rgb_light_color[2]},

		JSON_TO_FIELD(level_json, tone_mapping_max_white, float),
		JSON_TO_FIELD(level_json, noise_granularity, float)
	};

	//////////

	// TODO: put these in the JSON level file next
	static const GLchar* const sector_face_texture_paths[] = {
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

	static const GLchar* const still_billboard_texture_paths[] = {
		ASSET_PATH("objects/health_kit.bmp"),
		ASSET_PATH("objects/teleporter.bmp"),
		ASSET_PATH("objects/shabti.bmp")
	};

	static const AnimationLayout billboard_animation_layouts[] = {
		{ASSET_PATH("spritesheets/flying_carpet.bmp"), 5, 10, 46},
		{ASSET_PATH("spritesheets/torch_2.bmp"), 2, 3, 5},
		{ASSET_PATH("spritesheets/eddie.bmp"), 23, 1, 23},
		{ASSET_PATH("spritesheets/trooper.bmp"), 33, 1, 33},
		{ASSET_PATH("spritesheets/fireball_travel.bmp"), 12, 1, 12}
	};

	/* TODO:
	- Make these texture id ranges relative to each animation layout
	- Perhaps only apply one texture id looping construct per animation layout
		(sub-animations provided as different animation layouts) */
	static const Animation billboard_animations[] = {
		{.texture_id_range = {3, 48}, .secs_for_frame = 0.02f}, // Flying carpet
		{.texture_id_range = {49, 53}, .secs_for_frame = 0.15f}, // Torch
		{.texture_id_range = {62, 64}, .secs_for_frame = 0.08f}, // Eddie, attacking
		{.texture_id_range = {77, 81}, .secs_for_frame = 0.07f}, // Trooper, idle
		{.texture_id_range = {82, 88}, .secs_for_frame = 0.07f}, // Trooper, chasing
		{.texture_id_range = {89, 98}, .secs_for_frame = 0.07f}, // Trooper, attacking
		{.texture_id_range = {110, 121}, .secs_for_frame = 0.08f} // Traveling fireball
	};

	static const BillboardAnimationInstance billboard_animation_instances[] = {
		{.billboard_index = 10, .animation_index = 0}, // Flying carpet
		{.billboard_index = 11, .animation_index = 1}, // Torch

		{.billboard_index = 12, .animation_index = 2}, // Eddies
		{.billboard_index = 13, .animation_index = 2},

		{.billboard_index = 14, .animation_index = 3}, // Troopers
		{.billboard_index = 15, .animation_index = 4},
		{.billboard_index = 16, .animation_index = 5},

		{.billboard_index = 17, .animation_index = 6} // Traveling fireball
	};

	/* Still and animated billboards can be in any order (it's just easier to impose an order here).
	The material indices all start out as 0 (they are assigned in `init_materials_texture`).
	The animated billboard texture ids are assigned in the loop below the declaration of this array. */
	static Billboard billboards[] = {
		////////// Still billboards

		{0, 0, {1.0f, 1.0f}, {28.0f, 2.5f, 31.0f}}, // Health kits
		{0, 0, {1.0f, 1.0f}, {5.0f, 0.5f, 22.5f}},
		{0, 0, {1.0f, 1.0f}, {31.5f, 0.5f, 10.5f}},

		{0, 1, {1.0f, 1.0f}, {12.5f, 0.5f, 38.5f}}, // Teleporters
		{0, 1, {1.0f, 1.0f}, {8.5f, 0.5f, 25.5f}},
		{0, 1, {1.0f, 1.0f}, {32.5f, 2.5f, 7.5f}},

		{0, 2, {2.0f, 2.0f}, {4.5f, 4.0f, 12.5f}}, // Shabtis
		{0, 2, {2.0f, 2.0f}, {10.5f, 1.0f, 25.0f}},
		{0, 2, {2.0f, 2.0f}, {25.5f, 3.0f, 31.0f}},
		{0, 2, {4.0f, 4.0f}, {36.0f, 18.0f, 4.0f}},

		////////// Animated billboards

		{0, 0, {1.0f, 1.0f}, {5.0f, 0.5f, 2.0f}}, // Flying carpet
		{0, 0, {1.0f, 1.0f}, {7.5f, 0.5f, 12.5f}}, // Torch

		{0, 0, {1.0f, 1.0f}, {6.5f, 0.5f, 21.5f}}, // Eddies
		{0, 0, {1.0f, 1.0f}, {3.5f, 0.5f, 24.5f}},

		{0, 0, {1.0f, 1.0f}, {3.0f, 1.5f, 9.5f}}, // Troopers
		{0, 0, {1.0f, 1.0f}, {9.5f, 6.5f, 13.5f}},
		{0, 0, {1.0f, 1.0f}, {21.5f, 0.5f, 24.5f}},

		{0, 0, {5.0f, 4.0f}, {22.5f, 15.0f, 8.5}} // Traveling fireball
	};

	////////// Assigning initial texture ids to animated billboards

	for (billboard_index_t i = 0; i < ARRAY_LENGTH(billboard_animation_instances); i++) {
		const BillboardAnimationInstance* const animation_instance = billboard_animation_instances + i;

		billboards[animation_instance -> billboard_index].texture_id = billboard_animations[
			animation_instance -> animation_index].texture_id_range.start;
	}

	//////////

	// Note: the rescale size isn't used in `shared_material_properties`.
	const WeaponSpriteConfig weapon_sprite_config = {
		// Simple squares
		/*
		.max_degrees = {.yaw = 30.0f, .pitch = 120.0f},
		.secs_per = {.frame = 1.0f, .movement_cycle = 1.0f},
		.screen_space_size = 1.0f, .max_movement_magnitude = 0.4f,
		.animation_layout = {ASSET_PATH("walls/simple_squares.bmp"), 1, 1, 1},

		.shared_material_properties = {
			.bilinear_percents = {.albedo = 1.0f, .normal = 0.75f},
			.normal_map_config = {.blur_radius = 1, .blur_std_dev = 1.0f, .heightmap_scale = 1.0f, .rescale_factor = 2.0f}
		},

		.sound_path = ASSET_PATH("audio/sound_effects/health_increase.wav")
		*/

		// Desecrator
		/*
		.max_degrees = {.yaw = 20.0f, .pitch = 130.0f},
		.secs_per = {.frame = 0.07f, .movement_cycle = 1.0f},
		.screen_space_size = 0.7f, .max_movement_magnitude = 0.2f,
		.animation_layout = {ASSET_PATH("spritesheets/weapons/desecrator.bmp"), 1, 8, 8},

		.shared_material_properties = {
			.bilinear_percents = {.albedo = 0.5f, .normal = 0.5f},
			.normal_map_config = {.blur_radius = 0, .blur_std_dev = 0.0f, .heightmap_scale = 1.0f, .rescale_factor = 3.0f}
		},

		.sound_path = ASSET_PATH("audio/sound_effects/rocket_explosion.wav")
		*/

		// Whip
		.max_degrees = {.yaw = 15.0f, .pitch = 120.0f},
		.secs_per = {.frame = 0.02f, .movement_cycle = 0.9f},
		.screen_space_size = 0.75f, .max_movement_magnitude = 0.25f,
		.animation_layout = {ASSET_PATH("spritesheets/weapons/whip.bmp"), 4, 6, 22},

		.shared_material_properties = {
			.bilinear_percents = {.albedo = 1.0f, .normal = 1.0f},
			.normal_map_config = {.blur_radius = 0, .blur_std_dev = 0.0f, .heightmap_scale = 1.0f, .rescale_factor = 2.0f}
		},

		.sound_path = ASSET_PATH("audio/sound_effects/whip_crack.wav")

		// Snazzy shotgun
		/*
		.max_degrees = {.yaw = 30.0f, .pitch = 90.0f},
		.secs_per = {.frame = 0.035f, .movement_cycle = 0.9f},
		.screen_space_size = 0.75f, .max_movement_magnitude = 0.2f,
		.animation_layout = {ASSET_PATH("spritesheets/weapons/snazzy_shotgun.bmp"), 6, 10, 59},

		.shared_material_properties = {
			.bilinear_percents = {.albedo = 0.0f, .normal = 0.0f},
			.normal_map_config = {.blur_radius = 4, .blur_std_dev = 1.5f, .heightmap_scale = 2.0f, .rescale_factor = 2.0f}
		},

		.sound_path = ASSET_PATH("audio/sound_effects/shotgun.wav")
		*/

		// Reload pistol
		/*
		.max_degrees = {.yaw = 25.0f, .pitch = 90.0f},
		.secs_per = {.frame = 0.04f, .movement_cycle = 1.0f},
		.screen_space_size = 0.8f, .max_movement_magnitude = 0.2f,
		.animation_layout =  {ASSET_PATH("spritesheets/weapons/reload_pistol.bmp"), 4, 7, 28},

		.shared_material_properties = {
			.bilinear_percents = {.albedo = 0.5f, .normal = 1.0f},
			.normal_map_config = {.blur_radius = 2, .blur_std_dev = 1.0f, .heightmap_scale = 1.0f, .rescale_factor = 2.0f}
		},

		.sound_path = ASSET_PATH("audio/sound_effects/scrape.wav")
		*/
	};

	////////// Reading in all materials

	const char* const materials_file_path = ASSET_PATH("json_data/materials.json");
	cJSON* const materials_json = init_json_from_file(materials_file_path);

	const cJSON* json_material;

	List all_materials = init_list((buffer_size_t) cJSON_GetArraySize(materials_json), MaterialPropertiesPerObjectInstance);

	cJSON_ArrayForEach(json_material, materials_json) {
		vec3 lighting_props;
		read_floats_from_json_array(json_material, ARRAY_LENGTH(lighting_props), lighting_props);

		const MaterialPropertiesPerObjectInstance material = {
			.albedo_texture_path = json_material -> string,

			.lighting = {
				.metallicity = lighting_props[0],
				.min_roughness = lighting_props[1],
				.max_roughness = lighting_props[2]
			}
		};

		validate_material(material);

		push_ptr_to_list(&all_materials, &material);
	}

	////////// Making a materials texture

	/* `material_lighting_properties` is a list of lighting properties for the current level.
	Note that calling this also sets the material indices of billboards, and gets the weapon
	sprite material index too. */

	material_index_t weapon_sprite_material_index;

	const MaterialsTexture materials_texture = init_materials_texture(
		&all_materials,
		&(List) {(void*) sector_face_texture_paths, 	sizeof(*sector_face_texture_paths), ARRAY_LENGTH(sector_face_texture_paths), 0},
		&(List) {(void*) still_billboard_texture_paths, sizeof(*still_billboard_texture_paths), ARRAY_LENGTH(still_billboard_texture_paths), 0},
		&(List) {(void*) billboard_animation_layouts, 	sizeof(*billboard_animation_layouts), ARRAY_LENGTH(billboard_animation_layouts), 0},
		&(List) {(void*) billboards, 					sizeof(*billboards), ARRAY_LENGTH(billboards), 0},
		&weapon_sprite_config.animation_layout, &weapon_sprite_material_index
	);

	deinit_list(all_materials);
	deinit_json(materials_json);

	//////////

	const MaterialPropertiesPerObjectType
		sector_face_shared_material_properties = {
			.texture_rescale_size = 128,
			.bilinear_percents = {.albedo = 0.8f, .normal = 0.9f},

			.normal_map_config = {
				.use_anisotropic_filtering = true, .blur_radius = 3,
				.blur_std_dev = 0.8f, .heightmap_scale = 1.0f, .rescale_factor = 2.0f
			}
		},

		billboard_shared_material_properties = {
			.texture_rescale_size = 128,
			.bilinear_percents = {.albedo = 0.8f, .normal = 0.6f},

			.normal_map_config = {
				.use_anisotropic_filtering = true, .blur_radius = 1,
				.blur_std_dev = 0.1f, .heightmap_scale = 1.0f, .rescale_factor = 2.0f
			} // This, with 2x scaling, uses about 100mb more memory
		};

	////////// Making an array of all bilinear percents

	vec2 all_bilinear_percents[num_unique_object_types];

	const MaterialPropertiesPerObjectType* const all_shared_material_properties[num_unique_object_types] = {
		&sector_face_shared_material_properties,
		&billboard_shared_material_properties,
		&weapon_sprite_config.shared_material_properties
	};

	for (byte i = 0; i < num_unique_object_types; i++) {
		GLfloat* const bilinear_percents = all_bilinear_percents[i];
		const MaterialPropertiesPerObjectType* const shared_material_properties = all_shared_material_properties[i];

		bilinear_percents[0] = shared_material_properties -> bilinear_percents.albedo;
		bilinear_percents[1] = shared_material_properties -> bilinear_percents.normal;
	}

	//////////

	cJSON* const non_lighting_json = read_json_subobj(level_json, "non_lighting_data");

	specify_cascade_count_before_any_shader_compilation(
		window_config -> opengl_major_minor_version,
		level_rendering_config.shadow_mapping.cascaded_shadow_config.num_cascades);

	//////////

	const CameraConfig camera_config = {
		.init_pos = {1.5f, 0.5f, 1.5f}, // {0.5f, 0.0f, 0.5f},
		.angles = {.hori = ONE_FOURTH_PI, .vert = 0.0f, .tilt = 0.0f}
	};

	const ALchar* const level_soundtrack_path = get_string_from_json(read_json_subobj(non_lighting_json, "soundtrack_path"));

	//////////

	const byte
		// *const heightmap = (const byte*) blank_heightmap, *const texture_id_map = (const byte*) blank_texture_id_map, map_size[2] = {blank_width, blank_height};
		// *const heightmap = (const byte*) tiny_heightmap, *const texture_id_map = (const byte*) tiny_texture_id_map, map_size[2] = {tiny_width, tiny_height};
		// *const heightmap = (const byte*) checker_heightmap, *const texture_id_map = (const byte*) checker_texture_id_map, map_size[2] = {checker_width, checker_height};
		// *const heightmap = (const byte*) blank_heightmap, *const texture_id_map = (const byte*) blank_heightmap, map_size[2] = {blank_width, blank_height};
		*const heightmap = (const byte*) palace_heightmap, *const texture_id_map = (const byte*) palace_texture_id_map, map_size[2] = {palace_width, palace_height};
		// *const heightmap = (const byte*) fortress_heightmap, *const texture_id_map = (const byte*) fortress_texture_id_map, map_size[2] = {fortress_width, fortress_height};
		// *const heightmap = (const byte*) level_one_heightmap, *const texture_id_map = (const byte*) level_one_texture_id_map, map_size[2] = {level_one_width, level_one_height};
		// *const heightmap = (const byte*) terrain_heightmap, *const texture_id_map = (const byte*) terrain_texture_id_map, map_size[2] = {terrain_width, terrain_height};
		// *const heightmap = (const byte*) terrain_2_heightmap, *const texture_id_map = (const byte*) terrain_2_texture_id_map, map_size[2] = {terrain_2_width, terrain_2_height};

	const byte max_point_height = get_heightmap_max_point_height(heightmap, map_size);
	const GLfloat far_clip_dist = compute_world_far_clip_dist(map_size, max_point_height);

	//////////

	const struct {
		const TitleScreenTextureConfig texture;
		const TitleScreenRenderingConfig rendering;
	} title_screen_config = {
		.texture = {
			.paths = {.still = ASSET_PATH("logo.bmp"), .scrolling = ASSET_PATH("palace_city.bmp")},
			.mag_filters = {.still = TexNearest, .scrolling = TexLinear},
			.scrolling_normal_map_config = {.blur_radius = 5, .blur_std_dev = 0.25f, .heightmap_scale = 0.3f, .rescale_factor = 2.0f}
		},

		.rendering = {
			.texture_transition_immediacy_factor = 2,
			.scrolling_vert_squish_ratio = 0.5f,
			.specular_exponent = 16.0f,
			.scrolling_bilinear_albedo_percent = 0.1f,
			.scrolling_bilinear_normal_percent = 0.75f,
			.light_dist_from_screen_plane = 0.3f,
			.secs_per_scroll_cycle = 7.0f,
			.light_spin_cycle = {.secs_per = 2.5f, .logo_transitions_per = 0.5f}
		}
	};

	//////////

	const SceneContext scene_context = {
		.camera = init_camera(&camera_config, far_clip_dist),
		.materials_texture = materials_texture,
		.weapon_sprite = init_weapon_sprite(&weapon_sprite_config, weapon_sprite_material_index),

		.sector_context = init_sector_context(heightmap, texture_id_map,
			map_size[0], map_size[1], sector_face_texture_paths,
			ARRAY_LENGTH(sector_face_texture_paths), &sector_face_shared_material_properties,
			&level_rendering_config.dynamic_light_config
		),

		.billboard_context = init_billboard_context( // 0.2f before for the alpha threshold
			level_rendering_config.shadow_mapping.billboard_alpha_threshold,
			&billboard_shared_material_properties,

			ARRAY_LENGTH(billboard_animation_layouts), billboard_animation_layouts,

			ARRAY_LENGTH(still_billboard_texture_paths), still_billboard_texture_paths,
			ARRAY_LENGTH(billboards), billboards,
			ARRAY_LENGTH(billboard_animations), billboard_animations,
			ARRAY_LENGTH(billboard_animation_instances), billboard_animation_instances
		),

		.dynamic_light = init_dynamic_light(&level_rendering_config.dynamic_light_config),
		.shadow_context = init_shadow_context(&level_rendering_config.shadow_mapping.cascaded_shadow_config, far_clip_dist),

		.ao_map = init_ao_map(heightmap, map_size, max_point_height),
		.skybox = init_skybox(&level_rendering_config.skybox_config),
		.title_screen = init_title_screen(&title_screen_config.texture, &title_screen_config.rendering),
		.heightmap = heightmap, .map_size = {map_size[0], map_size[1]}
	};

	////////// Global state initialization

	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // This is correct for alpha premultiplication
	glDepthFunc(GL_LESS);

	/* Depth clamping is used for 1. shadow pancaking, 2. avoiding clipping with sectors when walking
	against them, and 3. stopping too much upwards weapon pitch from going through the near plane */
	const GLenum states[] = {GL_DEPTH_TEST, GL_DEPTH_CLAMP, GL_CULL_FACE, GL_TEXTURE_CUBE_MAP_SEAMLESS};
	for (byte i = 0; i < ARRAY_LENGTH(states); i++) glEnable(states[i]);

	////////// Initializing a scene context on the heap

	SceneContext* const scene_context_on_heap = alloc(1, sizeof(SceneContext));
	memcpy(scene_context_on_heap, &scene_context, sizeof(SceneContext));

	////////// Audio setup

	const ALchar
		*const jump_up_sound_path = ASSET_PATH("audio/sound_effects/jump_up.wav"),
		*const jump_land_sound_path = ASSET_PATH("audio/sound_effects/jump_land.wav");

	// TODO: return clip indices from these, that can be used to find the right audio source
	add_audio_clip_to_audio_context(&audio_context, weapon_sprite_config.sound_path, true);
	add_audio_clip_to_audio_context(&audio_context, jump_up_sound_path, true);
	add_audio_clip_to_audio_context(&audio_context, jump_land_sound_path, true);
	add_audio_clip_to_audio_context(&audio_context, level_soundtrack_path, false);

	// TODO: add a running sound
	const PositionalAudioSourceMetadata positional_audio_source_metadata[] = {
		{weapon_sprite_config.sound_path, &scene_context_on_heap -> weapon_sprite,
		weapon_sound_activator, weapon_sound_updater},

		{jump_up_sound_path, &scene_context_on_heap -> camera,
		jump_up_sound_activator, jump_up_sound_updater},

		{jump_land_sound_path, &scene_context_on_heap -> camera,
		jump_land_sound_activator, jump_land_sound_updater}
	};

	for (byte i = 0; i < ARRAY_LENGTH(positional_audio_source_metadata); i++)
		add_positional_audio_source_to_audio_context(&audio_context, positional_audio_source_metadata + i, false);

	// TODO: perhaps return source indices from this, that can be used to select a source to play
	add_nonpositional_audio_source_to_audio_context(&audio_context, level_soundtrack_path, true);
	play_nonpositional_audio_source(&audio_context, level_soundtrack_path);

	memcpy(&scene_context_on_heap -> audio_context, &audio_context, sizeof(AudioContext));

	////////// Initializing shared textures

	const WorldShadedObject world_shaded_objects[] = {
		{&scene_context.sector_context.drawable, {TU_SectorFaceAlbedo, TU_SectorFaceNormalMap}},
		{&scene_context.billboard_context.drawable, {TU_BillboardAlbedo, TU_BillboardNormalMap}},
		{&scene_context.weapon_sprite.drawable, {TU_WeaponSpriteAlbedo, TU_WeaponSpriteNormalMap}}
	};

	init_shared_textures_for_world_shaded_objects(world_shaded_objects,
		ARRAY_LENGTH(world_shaded_objects), &scene_context.shadow_context,
		&scene_context.ao_map, materials_texture.buffer_texture
	);

	////////// Initializing shared shading params

	const GLuint shaders_that_use_shared_params[] = {
		// Depth shaders
		scene_context.sector_context.depth_prepass_shader,
		scene_context.sector_context.shadow_mapping.depth_shader,
		scene_context.billboard_context.shadow_mapping.depth_shader,

		// Plain shaders
		scene_context.skybox.shader,
		scene_context.sector_context.drawable.shader,
		scene_context.billboard_context.drawable.shader,
		scene_context.weapon_sprite.drawable.shader
	};

	const SharedShadingParams shared_shading_params = init_shared_shading_params(
		shaders_that_use_shared_params, ARRAY_LENGTH(shaders_that_use_shared_params),
		&level_rendering_config, all_bilinear_percents, &scene_context.shadow_context
	);

	// I am bypassing the type system's const safety checks with this, but it's for the best
	memcpy(&scene_context_on_heap -> shared_shading_params, &shared_shading_params, sizeof(SharedShadingParams));

	//////////

	deinit_json(level_json);

	return scene_context_on_heap;
}

static void main_deinit(void* const app_context) {
	SceneContext* const scene_context = (SceneContext*) app_context;

	deinit_shared_shading_params(&scene_context -> shared_shading_params);
	deinit_materials_texture(&scene_context -> materials_texture);

	deinit_weapon_sprite(&scene_context -> weapon_sprite);
	deinit_sector_context(&scene_context -> sector_context);
	deinit_billboard_context(&scene_context -> billboard_context);

	deinit_ao_map(&scene_context -> ao_map);
	deinit_shadow_context(&scene_context -> shadow_context);
	deinit_title_screen(&scene_context -> title_screen);
	deinit_skybox(scene_context -> skybox);
	deinit_audio_context(&scene_context -> audio_context);

	dealloc(scene_context);
}

int main(void) {
	cJSON* const window_config_json = init_json_from_file(ASSET_PATH("json_data/window.json"));
	cJSON* const enabled_json = read_json_subobj(window_config_json, "enabled");

	//////////

	uint8_t opengl_major_minor_version[2];
	uint16_t window_size[2];

	GET_ARRAY_VALUES_FROM_JSON_KEY(window_config_json, opengl_major_minor_version, opengl_major_minor_version, u8);
	GET_ARRAY_VALUES_FROM_JSON_KEY(window_config_json, window_size, window_size, u16);

	//////////

	const WindowConfig window_config = {
		JSON_TO_FIELD(window_config_json, app_name, string),

		.enabled = {
			JSON_TO_FIELD(enabled_json, vsync, bool),
			JSON_TO_FIELD(enabled_json, aniso_filtering, bool),
			JSON_TO_FIELD(enabled_json, multisampling, bool),
			JSON_TO_FIELD(enabled_json, software_renderer, bool)
		},

		JSON_TO_FIELD(window_config_json, aniso_filtering_level, u8),
		JSON_TO_FIELD(window_config_json, multisample_samples, u8),
		JSON_TO_FIELD(window_config_json, default_fps, u8),
		JSON_TO_FIELD(window_config_json, depth_buffer_bits, u8),

		.opengl_major_minor_version = {
			opengl_major_minor_version[0],
			opengl_major_minor_version[1]
		},

		.window_size = {window_size[0], window_size[1]}
	};

	make_application(&window_config, main_drawer, main_init, main_deinit);
	deinit_json(window_config_json);
}
