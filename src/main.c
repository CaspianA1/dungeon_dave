#include "main.h"
#include "level_cache.h" // For `get_level_cache`
#include "utils/opengl_wrappers.h" // For OpenGL defs + wrappers
#include "utils/macro_utils.h" // For `ARRAY_LENGTH`
#include "data/constants.h" // For `num_unique_object_types`, `default_depth_func`, and `max_byte_value`
#include "utils/map_utils.h" // For `get_heightmap_max_point_height,` and `compute_world_far_clip_dist`
#include "utils/alloc.h" // For `alloc`, and `dealloc`
#include "window.h" // For `make_application`, and `WindowConfig`
#include "utils/debug_macro_utils.h" // For the debug keys, and `DEBUG_VEC3`

static LevelContext level_init(
	const GLchar* const level_path,
	PersistentGameContext* const persistent_game_context,
	LevelContext* const level_context_heap_dest) {

	////////// Printing library info

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

	reset_uniform_buffer_binding_point_counter();

	// This is correct for alpha premultiplication
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glDepthFunc(constants.default_depth_func);

	/* Depth clamping is used for 1. shadow pancaking, 2. avoiding clipping with sectors when walking
	against them, and 3. stopping too much upwards weapon pitch from going through the near plane */
	const GLenum enabled_states[] = {GL_DEPTH_TEST, GL_DEPTH_CLAMP, GL_CULL_FACE, GL_TEXTURE_CUBE_MAP_SEAMLESS};
	for (byte i = 0; i < ARRAY_LENGTH(enabled_states); i++) glEnable(enabled_states[i]);

	////////// Defining a bunch of level data

	/*
	How level data flow will happen:

	1. A function from each object interface will load an init object given a base key in the level struct.
		Only basic validation happens here; i.e. validation based around the value ranges required for fitting
		each number's value range into each struct member's type. Basically, this step will just create a valid struct
		given some JSON, and do a series of null and range checks. TODO: see if this is possible to automate somehow
		(or at least just a little).

	2. That init object will then passed to the initializer.
	3. Specific validation then happens in that initializer.

	Alternative idea: only pass the key to the 'init_*_context', and then do the parsing all in one function.
	But that will make passing a raw struct difficult to each context initializer.

	_____ Overall requirements for each context:

	1. Init base init object from JSON
	2. Init context from base init object
	3. Deinit context

	(Optional)
	4. Update
	5. Draw
	*/

	cJSON JSON_OBJ_NAME_DEF(level) = init_json_from_file(level_path);

	const cJSON
		DEF_JSON_SUBOBJ(level, parallax_mapping),
		DEF_JSON_SUBOBJ(level, shadow_mapping),
		DEF_JSON_SUBOBJ(level, volumetric_lighting),
		DEF_JSON_SUBOBJ(level, ambient_occlusion),
		DEF_JSON_SUBOBJ(level, dynamic_light),
		DEF_JSON_SUBOBJ(level, skybox),
		DEF_JSON_SUBOBJ(level, non_lighting_data);

	const cJSON
		DEF_JSON_SUBOBJ(shadow_mapping, shadow_cascades),
		DEF_JSON_SUBOBJ(ambient_occlusion, compute_config);

	//////////

	DEF_ARRAY_FROM_JSON(skybox, rotation_degrees_per_axis, float, float, 3);
	DEF_ARRAY_FROM_JSON(skybox, cylindrical_cap_blend_widths, float, float, 2);
	DEF_ARRAY_FROM_JSON(dynamic_light, unnormalized_from, float, possibly_negative_float, 3);
	DEF_ARRAY_FROM_JSON(dynamic_light, unnormalized_to, float, possibly_negative_float, 3);
	DEF_ARRAY_FROM_JSON(level, rgb_light_color, sdl_pixel_component_t, u8, 3);

	//////////

	// TODO: put more level rendering params in here
	const LevelRenderingConfig level_rendering_config = {
		.parallax_mapping = {
			JSON_TO_FIELD(parallax_mapping, enabled, bool),
			JSON_TO_FIELD(parallax_mapping, min_layers, float),
			JSON_TO_FIELD(parallax_mapping, max_layers, float),
			JSON_TO_FIELD(parallax_mapping, height_scale, float),
			JSON_TO_FIELD(parallax_mapping, lod_cutoff, float)
		},

		.shadow_mapping = {
			JSON_TO_FIELD(shadow_mapping, sample_radius, u8),
			JSON_TO_FIELD(shadow_mapping, esm_exponent, u8),

			JSON_TO_FIELD(shadow_mapping, esm_exponent_layer_scale_factor, float),
			JSON_TO_FIELD(shadow_mapping, billboard_alpha_threshold, float),

			// TODO: check that this is never equal to 0
			JSON_TO_FIELD(shadow_mapping, inter_cascade_blend_threshold, float),

			.cascaded_shadow_config = {
				JSON_TO_FIELD(shadow_cascades, num_cascades, u8),
				JSON_TO_FIELD(shadow_cascades, num_depth_buffer_bits, u8),
				JSON_TO_FIELD(shadow_cascades, resolution, u16),
				JSON_TO_FIELD(shadow_cascades, sub_frustum_scale, float),
				JSON_TO_FIELD(shadow_cascades, linear_split_weight, float)
			}
		},

		.volumetric_lighting = {
			JSON_TO_FIELD(volumetric_lighting, num_samples, u8),
			JSON_TO_FIELD(volumetric_lighting, sample_density, float),
			JSON_TO_FIELD(volumetric_lighting, opacity, float)
		},

		.ambient_occlusion = {
			JSON_TO_FIELD(ambient_occlusion, strength, float),

			.compute_config = {
				JSON_TO_FIELD(compute_config, workload_split_factor, u8),
				JSON_TO_FIELD(compute_config, num_trace_iters, u16),
				JSON_TO_FIELD(compute_config, max_num_ray_steps, u16)
			}
		},

		.dynamic_light_config = {
			JSON_TO_FIELD(dynamic_light, time_for_cycle, float),

			.unnormalized_from = {
				dynamic_light_unnormalized_from[0],
				dynamic_light_unnormalized_from[1],
				dynamic_light_unnormalized_from[2]
			},

			.unnormalized_to = {
				dynamic_light_unnormalized_to[0],
				dynamic_light_unnormalized_to[1],
				dynamic_light_unnormalized_to[2]
			},
		},

		.skybox_config = {
			JSON_TO_FIELD(skybox, texture_path, string),
			JSON_TO_FIELD(skybox, texture_scale, float),
			JSON_TO_FIELD(skybox, horizon_dist_scale, float),

			.rotation_degrees_per_axis = {
				skybox_rotation_degrees_per_axis[0],
				skybox_rotation_degrees_per_axis[1],
				skybox_rotation_degrees_per_axis[2]
			},

			.cylindrical_cap_blend_widths = {
				skybox_cylindrical_cap_blend_widths[0],
				skybox_cylindrical_cap_blend_widths[1]
			}
		},

		.rgb_light_color = {
			level_rgb_light_color[0],
			level_rgb_light_color[1],
			level_rgb_light_color[2]
		},

		JSON_TO_FIELD(level, tone_mapping_max_white, float),
		JSON_TO_FIELD(level, noise_granularity, float)
	};

	////////// Specifying the cascade count early on

	/* TODO: later on, don't do this; just make the geo shader instancing have 1 invocation, and make
	the normal instancing render to the different layers. That will allow for shader recompilation. */
	specify_cascade_count_before_any_shader_compilation(level_rendering_config.shadow_mapping.cascaded_shadow_config.num_cascades);

	////////// Loading in the heightmap and texture id map, validating them, and extracting data from them

	map_pos_xz_t heightmap_size, texture_id_map_size;

	map_pos_component_t* const EXTRACT_FROM_JSON_SUBOBJ(read_2D_map, non_lighting_data,
		heightmap_data,, sizeof(map_pos_component_t), &heightmap_size);

	map_pos_component_t* const EXTRACT_FROM_JSON_SUBOBJ(read_2D_map, non_lighting_data,
		texture_id_map_data,, sizeof(map_pos_component_t), &texture_id_map_size);

	for (byte i = 0; i < 2; i++) {
		const map_pos_component_t
			heightmap_size_component = get_indexed_map_pos_component(heightmap_size, i),
			texture_id_map_size_component = get_indexed_map_pos_component(texture_id_map_size, i);

		const GLchar* const axis_name = (i == 0) ? "width" : "height";

		if (heightmap_size_component != texture_id_map_size_component) FAIL(
			UseLevelHeightmap, "Cannot use the level heightmap "
			"because its %s is %u, while the texture id map's %s is %u",
			axis_name, heightmap_size_component, axis_name, texture_id_map_size_component);
	}

	const Heightmap heightmap = {heightmap_data, heightmap_size};
	const map_pos_component_t max_point_height = get_heightmap_max_point_height(heightmap);
	const GLfloat far_clip_dist = compute_world_far_clip_dist(heightmap.size, max_point_height);

	////////// Loading in the level cache

	const LevelCacheConfig level_cache_config = {
		.ambient_occlusion = {
			.heightmap = heightmap,
			.max_y = max_point_height,
			.compute_config = &level_rendering_config.ambient_occlusion.compute_config
		}
	};

	// Making a redundant vertex spec before the first OpenGL code, since one must always be active
	const GLuint redundant_vertex_spec = init_vertex_spec();
	use_vertex_spec(redundant_vertex_spec);

	const LevelCache level_cache = get_level_cache(level_path, &level_cache_config);

	////////// Reading in the sector face texture paths

	texture_id_t num_sector_face_texture_paths;

	const GLchar** const EXTRACT_FROM_JSON_SUBOBJ(read_string_vector,
		non_lighting_data, sector_face_texture_paths,, &num_sector_face_texture_paths);

	////////// Reading in the still billboard texture paths

	/* TODO: perhaps make a series of fns that init a certain context from just JSON
	(would that be in each context's src file? Or all in another parsing file?) */

	const cJSON DEF_JSON_SUBOBJ(non_lighting_data, billboard_data);

	texture_id_t num_still_billboard_texture_paths;

	const GLchar** const EXTRACT_FROM_JSON_SUBOBJ(read_string_vector, billboard_data,
		still_billboard_texture_paths,, &num_still_billboard_texture_paths);

	////////// Reading in the billboard animation layouts

	const cJSON DEF_JSON_SUBOBJ(billboard_data, animated_billboard_data);

	const billboard_index_t max_billboard_index = (billboard_index_t) ~0u;

	const billboard_index_t num_billboard_animations = validate_json_array(
		WITH_JSON_OBJ_SUFFIX(animated_billboard_data), -1, max_billboard_index);

	AnimationLayout* const billboard_animation_layouts = alloc(num_billboard_animations, sizeof(AnimationLayout));

	#define ANIMATION_LAYOUT_FIELD_CASE(index, field, typename_t)\
		case index: billboard_animation_layout -> field =\
		get_##typename_t##_from_json(WITH_JSON_OBJ_SUFFIX(item_in_layout)); break;

	JSON_FOR_EACH(i, billboard_animation_layout_data, animated_billboard_data,
		enum {num_fields_per_animation_layout = 5};
		validate_json_array(WITH_JSON_OBJ_SUFFIX(billboard_animation_layout_data), num_fields_per_animation_layout, UINT8_MAX);

		AnimationLayout* const billboard_animation_layout = billboard_animation_layouts + i;

		JSON_FOR_EACH(j, item_in_layout, billboard_animation_layout_data,
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
			// The material index is set in `init_materials_texture`
			.texture_id_range = {next_animated_frame_start, next_animated_frame_start + total_frames - 1},
			.secs_for_frame = layout -> secs_for_frame
		};

		next_animated_frame_start += total_frames;
	}

	////////// Reading in billboards

	const cJSON DEF_JSON_SUBOBJ(billboard_data, billboards);
	const GLchar* const billboard_categories[] = {"unanimated", "animated"};

	// Getting a total billboard count, and allocating a billboard array from that

	enum {num_billboard_categories = ARRAY_LENGTH(billboard_categories)};
	billboard_index_t num_billboards = 0, num_animated_billboards;

	// TODO: check that the number of billboards doesn't exceed the limit
	for (byte i = 0; i < num_billboard_categories; i++) {
		const cJSON JSON_OBJ_NAME_DEF(category) = read_json_subobj(
			WITH_JSON_OBJ_SUFFIX(billboards), billboard_categories[i]);

		const billboard_index_t num_billboards_in_category = validate_json_array(
			WITH_JSON_OBJ_SUFFIX(category), -1, max_billboard_index);

		// Category 1 is for animated billboards
		if (i == 1) num_animated_billboards = num_billboards_in_category;
		num_billboards += num_billboards_in_category;
	}

	BillboardAnimationInstance* const billboard_animation_instances = alloc(num_animated_billboards, sizeof(BillboardAnimationInstance));
	Billboard* const billboards = alloc(num_billboards, sizeof(Billboard));

	// Initializing the billboard array

	billboard_index_t billboard_index = 0, animation_instance_index = 0;
	for (byte category_index = 0; category_index < num_billboard_categories; category_index++) {
		const cJSON JSON_OBJ_NAME_DEF(category) = read_json_subobj(
			WITH_JSON_OBJ_SUFFIX(billboards), billboard_categories[category_index]);

		JSON_FOR_EACH(_, billboard_data, category,
			(void) _;

			enum {num_fields_per_billboard_json = 3};
			validate_json_array(WITH_JSON_OBJ_SUFFIX(billboard_data), num_fields_per_billboard_json, UINT8_MAX);

			// Note: the material index is set in `init_materials_texture`
			Billboard* const billboard = billboards + billboard_index;

			JSON_FOR_EACH(billboard_field_index, billboard_field, billboard_data,
				const cJSON* const named_billboard_field = WITH_JSON_OBJ_SUFFIX(billboard_field);

				switch (billboard_field_index) {
					case 0: { // Handling the texture or animation id
						texture_id_t
							texture_id_or_animation_index = get_u16_from_json(named_billboard_field),
							*const billboard_texture_id = &billboard -> curr_texture_id;

						const GLchar* const error_format_string = "%s billboard at index %u refers to an out-of-bounds %s of index %u";

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

								// TODO: what should I put for `cycle_start_time`?
								billboard_animation_instances[animation_instance_index] = (BillboardAnimationInstance) {
									billboard_index, texture_id_or_animation_index, 0.0f, false
								};

								break;
						}
						break;
					}
					case 1: billboard -> scale = get_float_from_json(named_billboard_field); break;
					case 2: read_floats_from_json_array(named_billboard_field, ARRAY_LENGTH(billboard -> pos), billboard -> pos); break;
				}
			);

			billboard_index++;
			if (category_index == 1) animation_instance_index++;
		);
	}

	////////// Defining a weapon sprite

	/* TODO: read in weapon sprite data from a non-level JSON file, and then
	to begin, select a weapon sprite by name in each level file */

	const cJSON DEF_JSON_SUBOBJ(non_lighting_data, weapon_sprite);

	const cJSON
			DEF_JSON_SUBOBJ(weapon_sprite, max_degrees),
			DEF_JSON_SUBOBJ(weapon_sprite, shared_material_properties),
			DEF_JSON_SUBOBJ(weapon_sprite, animation_layout);

	const cJSON
			DEF_JSON_SUBOBJ(shared_material_properties, bilinear_percents),
			DEF_JSON_SUBOBJ(shared_material_properties, normal_map);

	// Note: the rescale size isn't used in `shared_material_properties`.
	const WeaponSpriteConfig weapon_sprite_config = { // Whip
		.max_degrees = {
			JSON_TO_FIELD(max_degrees, yaw, float),
			JSON_TO_FIELD(max_degrees, pitch, float)
		},

		JSON_TO_FIELD(weapon_sprite, secs_per_movement_cycle, float),
		JSON_TO_FIELD(weapon_sprite, screen_space_size, float),
		JSON_TO_FIELD(weapon_sprite, max_movement_magnitude, float),

		.animation_layout = {
			JSON_TO_FIELD(animation_layout, spritesheet_path, string),
			JSON_TO_FIELD(animation_layout, frames_across, u16),
			JSON_TO_FIELD(animation_layout, frames_down, u16),
			JSON_TO_FIELD(animation_layout, total_frames, u16),
			JSON_TO_FIELD(animation_layout, secs_for_frame, float),
		},

		.shared_material_properties = {
			.bilinear_percents = {
				JSON_TO_FIELD(bilinear_percents, albedo, float),
				JSON_TO_FIELD(bilinear_percents, normal, float)
			},

			.normal_map_config = {
				JSON_TO_FIELD(normal_map, blur_radius, u8),
				JSON_TO_FIELD(normal_map, blur_std_dev, float),
				JSON_TO_FIELD(normal_map, heightmap_scale, float),
				JSON_TO_FIELD(normal_map, rescale_factor, float)
			}
		},

		JSON_TO_FIELD(weapon_sprite, sound_path, string)
	};

	////////// Reading in all materials

	cJSON JSON_OBJ_NAME_DEF(materials) = init_json_from_file("json_data/materials.json");

	Dict all_materials = init_dict((buffer_size_t) cJSON_GetArraySize(
		WITH_JSON_OBJ_SUFFIX(materials)), DV_String, DV_UnsignedInt);

	JSON_FOR_EACH(_, material_data, materials,
		(void) _;

		// TODO: also include a sound path per each material (different walking sounds)

		// TODO: figure out how to use `DEF_ARRAY_FROM_JSON` here

		vec3 normalized_properties;
		const byte num_normalized_properties = ARRAY_LENGTH(normalized_properties);

		read_floats_from_json_array(WITH_JSON_OBJ_SUFFIX(material_data),
			num_normalized_properties, normalized_properties);

		const GLchar
			*const property_names[] = {"metallicity", "min_roughness", "max_roughness"},
			*const albedo_texture_path = WITH_JSON_OBJ_SUFFIX(material_data) -> string;

		for (byte i = 0; i < num_normalized_properties; i++) {
			const GLfloat normalized_property = normalized_properties[i];

			// Not checking for if it's less than 0, since that's already handled by `read_floats_from_json_array`
			if (normalized_property > 1.0f) FAIL(InitializeMaterial,
				"Material property '%s' for texture path '%s' "
				"is %g, and outside of the expected [0, 1] domain",
				property_names[i], albedo_texture_path,
				(GLdouble) normalized_property
			);
		}

		//////////

		const GLfloat min_roughness = normalized_properties[1], max_roughness = normalized_properties[2];

		if (min_roughness > max_roughness)
			FAIL(InitializeMaterial, "Material for texture path '%s' cannot have its "
				"min roughness (%g) exceed its max roughness (%g)", albedo_texture_path,
				(GLdouble) min_roughness, (GLdouble) max_roughness
			);

		//////////

		const packed_material_properties_t properties = (packed_material_properties_t) (
			((byte) (normalized_properties[0] * constants.max_byte_value)) |
			(((byte) (min_roughness * constants.max_byte_value)) << 8u) |
			(((byte) (max_roughness * constants.max_byte_value)) << 16u)
		);

		typed_insert_into_dict(&all_materials, albedo_texture_path, properties, string, unsigned_int);
	);

	////////// Making a materials texture

	/* `material_lighting_properties` is a list of lighting properties for the current level.
	Note that calling this also sets the material indices of billboards, and gets the weapon
	sprite material index too. */

	material_index_t weapon_sprite_material_index;

	#define BUILD_TEMP_PTR_TO_LIST(contents_name) &(List) {(void*) (contents_name), sizeof(*contents_name), num_##contents_name, 0}

	const billboard_index_t num_billboard_animation_layouts = num_billboard_animations;

	const MaterialsTexture materials_texture = init_materials_texture(
		&all_materials,
		BUILD_TEMP_PTR_TO_LIST(sector_face_texture_paths),
		BUILD_TEMP_PTR_TO_LIST(still_billboard_texture_paths),
		BUILD_TEMP_PTR_TO_LIST(billboard_animation_layouts),
		BUILD_TEMP_PTR_TO_LIST(billboard_animations),
		BUILD_TEMP_PTR_TO_LIST(billboards),
		&weapon_sprite_config.animation_layout, &weapon_sprite_material_index
	);

	#undef BUILD_TEMP_PTR_TO_LIST

	deinit_dict(&all_materials);
	deinit_json(WITH_JSON_OBJ_SUFFIX(materials));

	////////// Defining shared material properties

	// TODO: put this in the level JSON files
	const MaterialPropertiesPerObjectType
		sector_face_shared_material_properties = {
			.texture_rescale_size = 128,
			.bilinear_percents = {.albedo = 0.8f, .normal = 0.9f},

			.normal_map_config = {
				.blur_radius = 3, .blur_std_dev = 0.8f,
				.heightmap_scale = 1.0f, .rescale_factor = 2.0f
			}
		},

		billboard_shared_material_properties = {
			.texture_rescale_size = 128,
			.bilinear_percents = {.albedo = 0.8f, .normal = 0.6f},

			.normal_map_config = {
				.blur_radius = 1, .blur_std_dev = 0.1f,
				.heightmap_scale = 1.0f, .rescale_factor = 2.0f
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

	////////// Defining the camera config

	const cJSON DEF_JSON_SUBOBJ(non_lighting_data, camera);

	DEF_ARRAY_FROM_JSON(camera, init_pos, float, float, 3);
	DEF_ARRAY_FROM_JSON(camera, angles_degrees, float, float, 3);

	const CameraConfig camera_config = {
		.init_pos = {camera_init_pos[0], camera_init_pos[1], camera_init_pos[2]},

		.angles = { // TODO: why can't I set an initial tilt angle?
			.hori = glm_rad(camera_angles_degrees[0]),
			.vert = glm_rad(camera_angles_degrees[1]),
			.tilt = glm_rad(camera_angles_degrees[2])
		}
	};

	//////////

	const NormalMapCreator* const normal_map_creator = &persistent_game_context -> normal_map_creator;

	//////////

	LevelContext level_context = {
		.level_json = WITH_JSON_OBJ_SUFFIX(level),

		.camera = init_camera(&camera_config, far_clip_dist),
		.materials_texture = materials_texture,

		.weapon_sprite = init_weapon_sprite(
			&weapon_sprite_config,
			normal_map_creator,
			weapon_sprite_material_index
		),

		.sector_context = init_sector_context(
			heightmap, texture_id_map_data,
			sector_face_texture_paths, num_sector_face_texture_paths,
			&sector_face_shared_material_properties,
			normal_map_creator,
			&level_rendering_config.dynamic_light_config
		),

		.billboard_context = init_billboard_context(
			level_rendering_config.shadow_mapping.billboard_alpha_threshold,
			&billboard_shared_material_properties,
			normal_map_creator,

			num_billboard_animations, billboard_animation_layouts,

			num_still_billboard_texture_paths, still_billboard_texture_paths,
			num_billboards, billboards,
			num_billboard_animations, billboard_animations,
			num_animated_billboards, billboard_animation_instances
		),

		.dynamic_light = init_dynamic_light(&level_rendering_config.dynamic_light_config),
		.shadow_context = init_shadow_context(&level_rendering_config.shadow_mapping.cascaded_shadow_config, far_clip_dist),

		.ao_map = level_cache.ao_map,

		.skybox = init_skybox(&level_rendering_config.skybox_config),
		.title_screen = init_title_screen_from_json("json_data/title_screen.json", normal_map_creator),
		.heightmap = heightmap
	};

	////////// Audio setup (TODO: put this data in some JSON file; perhaps `default_sounds.json`?)

	AudioContext* const audio_context = &persistent_game_context -> audio_context;

	const ALchar* const EXTRACT_FROM_JSON_SUBOBJ(get_string, non_lighting_data, soundtrack_path,);

	const ALchar
		*const jump_up_sound_path = "audio/sound_effects/jump_up.wav",
		*const jump_land_sound_path = "audio/sound_effects/jump_land.wav";

	// TODO: return clip indices from these, that can be used to find the right audio source
	add_audio_clip_to_audio_context(audio_context, weapon_sprite_config.sound_path, true);
	add_audio_clip_to_audio_context(audio_context, jump_up_sound_path, true);
	add_audio_clip_to_audio_context(audio_context, jump_land_sound_path, true);
	add_audio_clip_to_audio_context(audio_context, soundtrack_path, false);

	Camera* const camera_heap_dest = &level_context_heap_dest -> camera;
	WeaponSprite* const weapon_sprite_heap_dest = &level_context_heap_dest -> weapon_sprite;

	// TODO: add a running sound (probably for only above a certain speed)
	const PositionalAudioSourceMetadata positional_audio_source_metadata[] = {
		{weapon_sprite_config.sound_path, weapon_sprite_heap_dest,
		weapon_sound_activator, weapon_sound_updater},

		{jump_up_sound_path, camera_heap_dest, jump_up_sound_activator, jump_up_sound_updater},
		{jump_land_sound_path, camera_heap_dest, jump_land_sound_activator, jump_land_sound_updater}
	};

	for (byte i = 0; i < ARRAY_LENGTH(positional_audio_source_metadata); i++)
		add_positional_audio_source_to_audio_context(audio_context, positional_audio_source_metadata + i, false);

	// TODO: perhaps return source indices from this, that can be used to select a source to play
	add_nonpositional_audio_source_to_audio_context(audio_context, soundtrack_path, true);
	play_nonpositional_audio_source(audio_context, soundtrack_path);

	////////// Initializing shared textures

	const WorldShadedObject world_shaded_objects[] = {
		{&level_context.sector_context.drawable, {TU_SectorFaceAlbedo, TU_SectorFaceNormalMap}},
		{&level_context.billboard_context.drawable, {TU_BillboardAlbedo, TU_BillboardNormalMap}},
		{&level_context.weapon_sprite.drawable, {TU_WeaponSpriteAlbedo, TU_WeaponSpriteNormalMap}}
	};

	init_shared_textures_for_world_shaded_objects(world_shaded_objects,
		ARRAY_LENGTH(world_shaded_objects), &level_context.shadow_context,
		&level_context.ao_map, materials_texture.buffer_texture
	);

	////////// Initializing shared shading params

	const GLuint shaders_that_use_shared_params[] = {
		// Depth prepass shaders
		level_context.sector_context.depth_prepass_shader,
		level_context.weapon_sprite.depth_prepass_shader,

		// Depth shaders (for shadow mapping)
		level_context.sector_context.shadow_mapping.depth_shader,
		level_context.billboard_context.shadow_mapping.depth_shader,

		// Plain shaders
		level_context.skybox.drawable.shader,
		level_context.sector_context.drawable.shader,
		level_context.billboard_context.drawable.shader,
		level_context.weapon_sprite.drawable.shader
	};

	// TODO: init this before, as a part of the level context struct on the stack
	const SharedShadingParams shared_shading_params = init_shared_shading_params(
		shaders_that_use_shared_params, ARRAY_LENGTH(shaders_that_use_shared_params),
		&level_rendering_config, all_bilinear_percents, &level_context.shadow_context
	);

	// I am bypassing the type system's const safety checks with this, but it's for the best
	memcpy(&level_context.shared_shading_params, &shared_shading_params, sizeof(SharedShadingParams));

	////////// Some random deinit

	deinit_vertex_spec(redundant_vertex_spec);
	dealloc(billboard_animation_layouts);
	dealloc(still_billboard_texture_paths);
	dealloc(sector_face_texture_paths);
	dealloc(texture_id_map_data);

	//////////

	const GLchar* const GL_error = get_GL_error();
	if (strcmp(GL_error, "NO_ERROR")) FAIL(CreateLevel, "Could not create a level because of the following OpenGL error: '%s'", GL_error);

	//////////

	return level_context;
}

static void level_deinit(const LevelContext* const level_context) {
	dealloc(level_context -> heightmap.data);

	deinit_shared_shading_params(&level_context -> shared_shading_params);
	deinit_materials_texture(&level_context -> materials_texture);

	deinit_weapon_sprite(&level_context -> weapon_sprite);
	deinit_sector_context(&level_context -> sector_context);
	deinit_billboard_context(&level_context -> billboard_context);

	deinit_ao_map(&level_context -> ao_map);
	deinit_shadow_context(&level_context -> shadow_context);
	deinit_title_screen(&level_context -> title_screen);
	deinit_skybox(&level_context -> skybox);

	deinit_json(level_context -> level_json);
}

static bool level_drawer(LevelContext* const level_context,
	const PersistentGameContext* const persistent_game_context,
	const Event* const event) {

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

	if (tick_title_screen(&level_context -> title_screen, event))
		return true;

	////////// Some variable initialization

	const SectorContext* const sector_context = &level_context -> sector_context;
	const CascadedShadowContext* const shadow_context = &level_context -> shadow_context;

	Camera* const camera = &level_context -> camera;
	BillboardContext* const billboard_context = &level_context -> billboard_context;
	WeaponSprite* const weapon_sprite = &level_context -> weapon_sprite;

	DynamicLight* const dynamic_light = &level_context -> dynamic_light;
	const GLfloat* const dir_to_light = dynamic_light -> curr_dir, curr_time_secs = event -> curr_time_secs;

	////////// Scene updating

	update_camera(camera, event, &level_context -> heightmap);
	update_billboard_context(billboard_context, curr_time_secs);
	update_weapon_sprite(weapon_sprite, camera, event);
	update_dynamic_light(dynamic_light, curr_time_secs);
	update_shadow_context(shadow_context, camera, dir_to_light, event -> aspect_ratio);
	update_shared_shading_params(&level_context -> shared_shading_params, camera, shadow_context, dir_to_light);
	update_audio_context(&persistent_game_context -> audio_context, camera); // TODO: should this be called here, or in `game_drawer`?

	////////// Rendering to the shadow context

	// TODO: still enable face culling for sectors?
	WITHOUT_BINARY_RENDER_STATE(GL_CULL_FACE,
		enable_rendering_to_shadow_context(shadow_context);
			draw_sectors_to_shadow_context(sector_context);
			draw_billboards_to_shadow_context(billboard_context);
		disable_rendering_to_shadow_context(event -> screen_size);
	);

	////////// The main drawing code

	draw_weapon_sprite_for_depth_prepass(weapon_sprite);
	draw_sectors(sector_context, camera);

	// No backface culling or depth buffer writes for the skybox, billboards, or weapon sprite
	WITHOUT_BINARY_RENDER_STATE(GL_CULL_FACE,
		WITH_RENDER_STATE(glDepthMask, GL_FALSE, GL_TRUE,
			draw_skybox(&level_context -> skybox); // Drawn before any translucent geometry

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

//////////

static void* game_init(void) {
	GameContext* const game_context_on_heap = alloc(1, sizeof(GameContext));

	/* TODO: clear the audio context between levels,
	or split the audio context up into 2 parts:
	the OpenAL part, and the state dictionaries. */

	const GLchar* const level_path = "json_data/levels/palace.json";

	////////// Loading the level context

	PersistentGameContext persistent_game_context = {
		.audio_context = init_audio_context(),
		.normal_map_creator = init_normal_map_creator()
	};

	const LevelContext curr_level_context = level_init(
		level_path,
		&persistent_game_context,
		&game_context_on_heap -> curr_level_context
	);

	const GameContext game_context = {
		.persistent_game_context = persistent_game_context,
		.curr_level_context = curr_level_context
	};

	memcpy(game_context_on_heap, &game_context, sizeof(GameContext));
	return game_context_on_heap;
}

static void game_deinit(void* const app_context) {
	GameContext* const game_context = app_context;
	level_deinit(&game_context -> curr_level_context);

	/* At this point, the data stored in `PositionalAudioSourceMetadata` within
	the audio context may have been deallocated, so that should not be accessed
	at this point. The same goes for all dict keys in the audio context. */

	PersistentGameContext* const persistent_game_context = &game_context -> persistent_game_context;

	deinit_normal_map_creator(&persistent_game_context -> normal_map_creator);
	deinit_audio_context(&persistent_game_context -> audio_context);
	dealloc(game_context);
}

static bool game_drawer(void* const app_context, const Event* const event) {
	GameContext* const game_context = app_context;
	LevelContext* const curr_level_context = &game_context -> curr_level_context;

	// TODO: don't hardcode this key
	if (event -> keys[SDL_SCANCODE_C]) {
		level_deinit(curr_level_context);

		// TODO: store the next level path in the curr level JSON instead
		const GLchar* const next_level_path = "json_data/levels/mountain.json";

		PersistentGameContext* const persistent_game_context = &game_context -> persistent_game_context;

		reset_audio_context(&persistent_game_context -> audio_context);

		const LevelContext next_level_context = level_init(
			next_level_path, persistent_game_context, curr_level_context
		);

		memcpy(curr_level_context, &next_level_context, sizeof(LevelContext));
	}

	return level_drawer(
		curr_level_context,
		&game_context -> persistent_game_context,
		event
	);
}

//////////

static void* cjson_wrapping_alloc(const size_t size) {
	return alloc(size, 1);
}

int main(void) {
	// This is just for tracking cJSON allocations
	cJSON_InitHooks(&(cJSON_Hooks) {cjson_wrapping_alloc, dealloc});

	////////// Reading in the window config

	cJSON JSON_OBJ_NAME_DEF(window_config) = init_json_from_file("json_data/window.json");
	const cJSON DEF_JSON_SUBOBJ(window_config, enabled);

	////////// Reading in some arrays

	DEF_ARRAY_FROM_JSON(window_config, opengl_major_minor_version, uint8_t, u8, 2);
	DEF_ARRAY_FROM_JSON(window_config, window_size, uint16_t, u16, 2);

	////////// Making a window config

	const WindowConfig window_config = {
		JSON_TO_FIELD(window_config, app_name, string),

		.enabled = {
			JSON_TO_FIELD(enabled, vsync, bool),
			JSON_TO_FIELD(enabled, aniso_filtering, bool),
			JSON_TO_FIELD(enabled, multisampling, bool),
			JSON_TO_FIELD(enabled, software_renderer, bool)
		},

		JSON_TO_FIELD(window_config, aniso_filtering_level, u8),
		JSON_TO_FIELD(window_config, multisample_samples, u8),
		JSON_TO_FIELD(window_config, default_fps, u8),
		JSON_TO_FIELD(window_config, depth_buffer_bits, u8),

		.opengl_major_minor_version = {
			window_config_opengl_major_minor_version[0],
			window_config_opengl_major_minor_version[1]
		},

		.window_size = {window_config_window_size[0], window_config_window_size[1]}
	};

	make_application(&window_config, game_init, game_deinit, game_drawer);

	// This is deinited after `make_application` because of the lifetime of `app_name`
	deinit_json(WITH_JSON_OBJ_SUFFIX(window_config));
}
