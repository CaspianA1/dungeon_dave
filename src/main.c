#include "main.h"
#include "utils/opengl_wrappers.h" // For OpenGL defs + wrappers
#include "utils/macro_utils.h" // For `ASSET_PATH`, and `ARRAY_LENGTH`
#include "data/constants.h" // For `num_unique_object_types`
#include "utils/map_utils.h" // For `get_heightmap_max_point_height,` and `compute_world_far_clip_dist`
#include "utils/alloc.h" // For `alloc`, and `dealloc`
#include "window.h" // For `make_application`, and `WindowConfig`
#include "utils/json.h" // For various JSON defs
#include "utils/debug_macro_utils.h" // For the debug keys, and `DEBUG_VEC3`

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

	// No backface culling or depth buffer writes for the skybox, billboards, or weapon sprite
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

	////////// Global state initialization

	// This is correct for alpha premultiplication
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glDepthFunc(GL_LESS);

	/* Depth clamping is used for 1. shadow pancaking, 2. avoiding clipping with sectors when walking
	against them, and 3. stopping too much upwards weapon pitch from going through the near plane */
	const GLenum states[] = {GL_DEPTH_TEST, GL_DEPTH_CLAMP, GL_CULL_FACE, GL_TEXTURE_CUBE_MAP_SEAMLESS};
	for (byte i = 0; i < ARRAY_LENGTH(states); i++) glEnable(states[i]);

	////////// Defining a bunch of level data

	cJSON* const level_json = init_json_from_file(ASSET_PATH("json_data/levels/palace.json"));

	const cJSON // TODO: genericize this naming thing here via a macro
		*const parallax_json = read_json_subobj(level_json, "parallax_mapping"),
		*const shadow_mapping_json = read_json_subobj(level_json, "shadow_mapping"),
		*const vol_lighting_json = read_json_subobj(level_json, "volumetric_lighting"),
		*const ao_json = read_json_subobj(level_json, "ambient_occlusion"),
		*const dyn_light_json = read_json_subobj(level_json, "dynamic_light"),
		*const skybox_json = read_json_subobj(level_json, "skybox"),
		*const non_lighting_json = read_json_subobj(level_json, "non_lighting_data");

	const cJSON
		*const dyn_light_looking_at_json = read_json_subobj(dyn_light_json, "looking_at"),
		*const cascaded_shadow_json = read_json_subobj(shadow_mapping_json, "cascades");

	vec3 dyn_light_pos, dyn_light_looking_at_origin, dyn_light_looking_at_dest;
	sdl_pixel_component_t rgb_light_color[3];

	GET_ARRAY_VALUES_FROM_JSON_KEY(dyn_light_json, dyn_light_pos, pos, float);
	GET_ARRAY_VALUES_FROM_JSON_KEY(dyn_light_looking_at_json, dyn_light_looking_at_origin, origin, float);
	GET_ARRAY_VALUES_FROM_JSON_KEY(dyn_light_looking_at_json, dyn_light_looking_at_dest, dest, float);
	GET_ARRAY_VALUES_FROM_JSON_KEY(level_json, rgb_light_color, rgb_light_color, u8);

	////////// Loading in the heightmap and texture id map, validating them, and extracting data from them

	byte map_size[2], cmp_map_size[2];

	byte
		*const heightmap = read_2D_map_from_json(read_json_subobj(non_lighting_json, "heightmap"), map_size),
		*const texture_id_map = read_2D_map_from_json(read_json_subobj(non_lighting_json, "texture_id_map"), cmp_map_size);

	for (byte i = 0; i < ARRAY_LENGTH(map_size); i++) {
		const byte size_component = map_size[i], cmp_size_component = cmp_map_size[i];

		const GLchar* const axis_name = (i == 0) ? "width" : "height";

		if (size_component != cmp_size_component) FAIL(
			UseLevelHeightmap, "Cannot use the level heightmap "
			"because its %s is %hhu, while the texture id map's %s is %hhu",
			axis_name, size_component, axis_name, cmp_size_component);
	}

	const byte max_point_height = get_heightmap_max_point_height(heightmap, map_size);
	const GLfloat far_clip_dist = compute_world_far_clip_dist(map_size, max_point_height);

	//////////

	const cJSON* const skybox_spherical_distortion_config_json = read_json_subobj(skybox_json, "spherical_distortion_config");
	const SkyboxSphericalDistortionConfig* skybox_spherical_distortion_config_ref = NULL;

	if (!cJSON_IsNull(skybox_spherical_distortion_config_json)) {
		static SkyboxSphericalDistortionConfig skybox_spherical_distortion_config;

		skybox_spherical_distortion_config = (SkyboxSphericalDistortionConfig) {
			.level_size = {map_size[0], max_point_height, map_size[1]},
			JSON_TO_FIELD(skybox_spherical_distortion_config_json, output_texture_scale, float),
			JSON_TO_FIELD(skybox_spherical_distortion_config_json, percentage_towards_y_top, float)
		};

		GET_ARRAY_VALUES_FROM_JSON_KEY(skybox_spherical_distortion_config_json,
			skybox_spherical_distortion_config.scale_ratios, scale_ratios, float);

		skybox_spherical_distortion_config_ref = &skybox_spherical_distortion_config;
	}

	//////////

	// TODO: put more level rendering params in here
	const LevelRenderingConfig level_rendering_config = {
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

			// TODO: check that this is never equal to 0
			JSON_TO_FIELD(shadow_mapping_json, inter_cascade_blend_threshold, float),

			.cascaded_shadow_config = {
				JSON_TO_FIELD(cascaded_shadow_json, num_cascades, u8),
				JSON_TO_FIELD(cascaded_shadow_json, num_depth_buffer_bits, u8),
				JSON_TO_FIELD(cascaded_shadow_json, resolution, u16),
				JSON_TO_FIELD(cascaded_shadow_json, sub_frustum_scale, float),
				JSON_TO_FIELD(cascaded_shadow_json, linear_split_weight, float)
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
			.spherical_distortion_config = skybox_spherical_distortion_config_ref
		},

		.rgb_light_color = {rgb_light_color[0], rgb_light_color[1], rgb_light_color[2]},

		JSON_TO_FIELD(level_json, tone_mapping_max_white, float),
		JSON_TO_FIELD(level_json, noise_granularity, float)
	};

	////////// Reading in the sector face texture paths

	texture_id_t num_sector_face_texture_paths;

	const GLchar** const sector_face_texture_paths = read_string_vector_from_json(
		read_json_subobj(non_lighting_json, "sector_face_texture_paths"), &num_sector_face_texture_paths);

	////////// Reading in the still billboard texture paths

	// TODO: perhaps make a series of fns that init a certain context from just JSON (would that be in each context's src file? Or all in another parsing file?)
	const cJSON* const billboard_data_json = read_json_subobj(non_lighting_json, "billboard_data");
	const cJSON* const still_billboard_texture_paths_json = read_json_subobj(billboard_data_json, "still_billboard_textures");

	texture_id_t num_still_billboard_texture_paths;
	const GLchar** const still_billboard_texture_paths = read_string_vector_from_json(still_billboard_texture_paths_json, &num_still_billboard_texture_paths);

	////////// Reading in the billboard animation layouts

	const cJSON* const billboard_animation_layouts_json = read_json_subobj(billboard_data_json, "animated_billboard_textures");

	const billboard_index_t max_billboard_index = (billboard_index_t) ~0u;
	const billboard_index_t num_billboard_animations = validate_json_array(billboard_animation_layouts_json, -1, max_billboard_index);

	AnimationLayout* const billboard_animation_layouts = alloc(num_billboard_animations, sizeof(AnimationLayout));

	#define ANIMATION_LAYOUT_FIELD_CASE(index, field, type_t)\
		case index: billboard_animation_layout -> field = get_##type_t##_from_json(item_in_layout_json); break;

	JSON_FOR_EACH(i, billboard_animation_layout_json, billboard_animation_layouts_json,
		enum {num_fields_per_animation_layout = 5};
		validate_json_array(billboard_animation_layout_json, num_fields_per_animation_layout, UINT8_MAX);

		AnimationLayout* const billboard_animation_layout = billboard_animation_layouts + i;

		JSON_FOR_EACH(j, item_in_layout_json, billboard_animation_layout_json,
			switch (j) {
				ANIMATION_LAYOUT_FIELD_CASE(0, spritesheet_path, string)
				ANIMATION_LAYOUT_FIELD_CASE(1, frames_across, u16)
				ANIMATION_LAYOUT_FIELD_CASE(2, frames_down, u16)
				ANIMATION_LAYOUT_FIELD_CASE(3, total_frames, u16)
				ANIMATION_LAYOUT_FIELD_CASE(4, secs_for_frame, float)
			}
		);
	);

	#undef ANIMATION_LAYOUT_FIELD_CASE

	////////// Making a series of billboard animations from the billboard animation layouts

	Animation* const billboard_animations = alloc(num_billboard_animations, sizeof(Animation));
	texture_id_t next_animated_frame_start = num_still_billboard_texture_paths;

	for (billboard_index_t i = 0; i < num_billboard_animations; i++) {
		const AnimationLayout* const layout = billboard_animation_layouts + i;
		const texture_id_t total_frames = layout -> total_frames;

		billboard_animations[i] = (Animation) {
			.texture_id_range = {next_animated_frame_start, next_animated_frame_start + total_frames - 1},
			.secs_for_frame = layout -> secs_for_frame
		};

		next_animated_frame_start += total_frames;
	}

	////////// Reading in billboards

	const cJSON* const billboards_json = read_json_subobj(billboard_data_json, "billboards");
	const GLchar* const billboard_categories[] = {"unanimated", "animated"};

	// Getting a total billboard count, and allocating a billboard array from that

	enum {num_billboard_categories = ARRAY_LENGTH(billboard_categories)};
	billboard_index_t num_billboards = 0, num_billboards_per_category[num_billboard_categories];

	// TODO: check that the number of billboards doesn't exceed the limit
	for (byte i = 0; i < num_billboard_categories; i++) {
		const cJSON* const category_json = read_json_subobj(billboards_json, billboard_categories[i]);
		const billboard_index_t num_billboards_in_category = validate_json_array(category_json, -1, max_billboard_index);
		num_billboards += (num_billboards_per_category[i] = num_billboards_in_category);
	}

	const billboard_index_t num_animated_billboards = num_billboards_per_category[1];

	BillboardAnimationInstance* const billboard_animation_instances = alloc(num_animated_billboards, sizeof(BillboardAnimationInstance));
	Billboard* const billboards = alloc(num_billboards, sizeof(Billboard));

	// Initializing the billboard array

	billboard_index_t billboard_index = 0, animation_instance_index = 0;
	for (byte category_index = 0; category_index < num_billboard_categories; category_index++) {
		const cJSON* const category_json = read_json_subobj(billboards_json, billboard_categories[category_index]);

		JSON_FOR_EACH(_, billboard_json, category_json,
			enum {num_fields_per_billboard_json = 3};
			validate_json_array(billboard_json, num_fields_per_billboard_json, UINT8_MAX);

			// Note: the material index is set in `init_materials_texture`
			Billboard* const billboard = billboards + billboard_index;

			JSON_FOR_EACH(billboard_field_index, billboard_field, billboard_json,
				switch (billboard_field_index) {
					case 0: { // Handling the texture or animation id
						texture_id_t
							texture_id_or_animation_index = get_u16_from_json(billboard_field),
							*const billboard_texture_id = &billboard -> texture_id;

						const GLchar* const error_format_string = "%s billboard at index %hu refers to an out-of-bounds %s of index %hu";

						switch (category_index) {
							case 0: // Unanimated
								if (texture_id_or_animation_index >= num_still_billboard_texture_paths) FAIL(
									MakeBillboard, error_format_string, "Still", billboard_index,
									"still texture", texture_id_or_animation_index
								);

								*billboard_texture_id = texture_id_or_animation_index;
								break;

							case 1: // Animated
								if (texture_id_or_animation_index >= num_billboard_animations) FAIL(
									MakeBillboard, error_format_string, "Animated", animation_instance_index,
									"animation", texture_id_or_animation_index
								);

								*billboard_texture_id = billboard_animations[texture_id_or_animation_index].texture_id_range.start;

								billboard_animation_instances[animation_instance_index] =
									(BillboardAnimationInstance) {billboard_index, texture_id_or_animation_index};

								break;
						}
						break;
					}
					case 1: billboard -> scale = get_float_from_json(billboard_field); break;
					case 2: read_floats_from_json_array(billboard_field, ARRAY_LENGTH(billboard -> pos), billboard -> pos); break;
				}
			);

			billboard_index++;
			if (category_index == 1) animation_instance_index++;

		);
	}

	////////// Defining a weapon sprite

	// Note: the rescale size isn't used in `shared_material_properties`.
	const WeaponSpriteConfig weapon_sprite_config = { // Whip
		.max_degrees = {.yaw = 15.0f, .pitch = 120.0f},
		.secs_per_movement_cycle = 0.9f, .screen_space_size = 0.75f, .max_movement_magnitude = 0.25f,
		.animation_layout = {ASSET_PATH("spritesheets/weapons/whip.bmp"), 4, 6, 22, 0.02f},

		.shared_material_properties = {
			.bilinear_percents = {.albedo = 1.0f, .normal = 1.0f},
			.normal_map_config = {.blur_radius = 0, .blur_std_dev = 0.0f, .heightmap_scale = 1.0f, .rescale_factor = 2.0f}
		},

		.sound_path = ASSET_PATH("audio/sound_effects/whip_crack.wav")
	};

	////////// Reading in all materials

	const GLchar* const materials_file_path = ASSET_PATH("json_data/materials.json");
	cJSON* const materials_json = init_json_from_file(materials_file_path);

	Dict all_materials = init_dict((buffer_size_t) cJSON_GetArraySize(materials_json), DV_String, DV_UnsignedInt);

	JSON_FOR_EACH(_, json_material, materials_json,
		vec3 normalized_properties;
		const byte num_normalized_properties = ARRAY_LENGTH(normalized_properties);

		read_floats_from_json_array(json_material, num_normalized_properties, normalized_properties);

		const GLchar
			*const property_names[] = {"metallicity", "min_roughness", "max_roughness"},
			*const albedo_texture_path = json_material -> string;

		for (byte i = 0; i < num_normalized_properties; i++) {
			const GLfloat normalized_property = normalized_properties[i];

			if (normalized_property < 0.0f || normalized_property > 1.0f)
				FAIL(InitializeMaterial, "Material property '%s' for texture path '%s' "\
					"is %g, and outside of the expected [0, 1] domain",
					property_names[i], albedo_texture_path, (GLdouble) normalized_property
				);
		}

		//////////

		const packed_material_properties_t properties = (packed_material_properties_t) (
			((byte) (normalized_properties[0] * constants.max_byte_value)) |
			(((byte) (normalized_properties[1] * constants.max_byte_value)) << 8u) |
			(((byte) (normalized_properties[2] * constants.max_byte_value)) << 16u)
		);

		typed_insert_into_dict(&all_materials, albedo_texture_path, properties, string, unsigned_int);
	);

	////////// Making a materials texture

	/* `material_lighting_properties` is a list of lighting properties for the current level.
	Note that calling this also sets the material indices of billboards, and gets the weapon
	sprite material index too. */

	material_index_t weapon_sprite_material_index;

	const MaterialsTexture materials_texture = init_materials_texture(
		&all_materials,
		&(List) {(void*) sector_face_texture_paths, 	sizeof(*sector_face_texture_paths), num_sector_face_texture_paths, 0},
		&(List) {(void*) still_billboard_texture_paths, sizeof(*still_billboard_texture_paths), num_still_billboard_texture_paths, 0},
		&(List) {(void*) billboard_animation_layouts, 	sizeof(*billboard_animation_layouts), num_billboard_animations, 0},
		&(List) {(void*) billboards, 					sizeof(*billboards), num_billboards, 0},
		&weapon_sprite_config.animation_layout, &weapon_sprite_material_index
	);

	deinit_dict(&all_materials);
	deinit_json(materials_json);

	////////// Defining shared material properties

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

	specify_cascade_count_before_any_shader_compilation(
		window_config -> opengl_major_minor_version,
		level_rendering_config.shadow_mapping.cascaded_shadow_config.num_cascades);

	//////////

	const CameraConfig camera_config = {
		.init_pos = {1.5f, 0.5f, 1.5f}, // {0.5f, 0.0f, 0.5f},
		.angles = {.hori = ONE_FOURTH_PI, .vert = 0.0f, .tilt = 0.0f}
	};

	const ALchar* const level_soundtrack_path = get_string_from_json(read_json_subobj(non_lighting_json, "soundtrack_path"));

	////////// Defining the title screen config

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
			num_sector_face_texture_paths, &sector_face_shared_material_properties,
			&level_rendering_config.dynamic_light_config
		),

		.billboard_context = init_billboard_context(
			level_rendering_config.shadow_mapping.billboard_alpha_threshold,
			&billboard_shared_material_properties,

			num_billboard_animations, billboard_animation_layouts,

			num_still_billboard_texture_paths, still_billboard_texture_paths,
			num_billboards, billboards,
			num_billboard_animations, billboard_animations,
			num_animated_billboards, billboard_animation_instances
		),

		.dynamic_light = init_dynamic_light(&level_rendering_config.dynamic_light_config),
		.shadow_context = init_shadow_context(&level_rendering_config.shadow_mapping.cascaded_shadow_config, far_clip_dist),

		.ao_map = init_ao_map(heightmap, map_size, max_point_height),
		.skybox = init_skybox(&level_rendering_config.skybox_config),
		.title_screen = init_title_screen(&title_screen_config.texture, &title_screen_config.rendering),
		.heightmap = heightmap, .map_size = {map_size[0], map_size[1]}
	};

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
		scene_context.skybox.drawable.shader,
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

	////////// Some random deinit

	dealloc(billboard_animation_layouts);
	dealloc(still_billboard_texture_paths);
	dealloc(sector_face_texture_paths);
	dealloc(texture_id_map);
	deinit_json(level_json);

	return scene_context_on_heap;
}

static void main_deinit(void* const app_context) {
	SceneContext* const scene_context = (SceneContext*) app_context;

	dealloc(scene_context -> heightmap);

	deinit_shared_shading_params(&scene_context -> shared_shading_params);
	deinit_materials_texture(&scene_context -> materials_texture);

	deinit_weapon_sprite(&scene_context -> weapon_sprite);
	deinit_sector_context(&scene_context -> sector_context);
	deinit_billboard_context(&scene_context -> billboard_context);

	deinit_ao_map(&scene_context -> ao_map);
	deinit_shadow_context(&scene_context -> shadow_context);
	deinit_title_screen(&scene_context -> title_screen);
	deinit_skybox(&scene_context -> skybox);
	deinit_audio_context(&scene_context -> audio_context);

	dealloc(scene_context);
}

static void* wrapping_alloc(const size_t size) {
	return alloc(size, 1);
}

int main(void) {
	// This is just for tracking cJSON allocations
	cJSON_InitHooks(&(cJSON_Hooks) {wrapping_alloc, dealloc});

	////////// Reading in the window config

	cJSON* const window_config_json = init_json_from_file(ASSET_PATH("json_data/window.json"));
	const cJSON* const enabled_json = read_json_subobj(window_config_json, "enabled");

	////////// Reading in some arrays

	uint8_t opengl_major_minor_version[2];
	uint16_t window_size[2];

	GET_ARRAY_VALUES_FROM_JSON_KEY(window_config_json, opengl_major_minor_version, opengl_major_minor_version, u8);
	GET_ARRAY_VALUES_FROM_JSON_KEY(window_config_json, window_size, window_size, u16);

	////////// Making a window config

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
