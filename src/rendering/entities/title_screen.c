#include "rendering/entities/title_screen.h"
#include "utils/macro_utils.h" // For `CHECK_BITMASK`
#include "utils/opengl_wrappers.h" // For `INIT_UNIFORM_ID`, `INIT_UNIFORM_VALUE`, and `UPDATE_UNIFORM`
#include "utils/json.h" // For various JSON defs
#include "utils/shader.h" // For `init_shader`

/* TODO: rescale the title screen's drawn aspect ratio for wider screens.
For the base logo, keep it from being stretched or squished, but extend/shrink
the other image for wider/narrower screen configurations. */

////////// Uniform updating

typedef struct {
	const TitleScreen* const title_screen;
	const GLfloat curr_time_secs;
} UniformUpdaterParams;

static void update_uniforms(const void* const param) {
	const UniformUpdaterParams typed_params = *(UniformUpdaterParams*) param;
	const TitleScreen* const title_screen = typed_params.title_screen;
	const TitleScreenConfig* const config = &title_screen -> config;

	const GLfloat time_seed = typed_params.curr_time_secs;

	const GLfloat // Modding the hori scroll with 1 to avoid floating-point imprecision errors for large time values
		palace_city_hori_scroll = fmodf(time_seed / config -> shared.secs_per_scroll_cycle, 1.0f),
		spin_seed = time_seed * TWO_PI / config -> shared.light_spin_cycle.secs_per;

	/* TODO: if used, make this focus on different quadrants each time
	(and make sure that `secs_per` is still followed) */
	// spin_seed += tanf(sinf(spin_seed));

	GLfloat texture_transition_weight = cosf(
		spin_seed * config -> shared.light_spin_cycle.logo_transitions_per
	) * 0.5f + 0.5f;

	for (byte i = 0; i < config -> shared.texture_transition_immediacy_factor; i++)
		texture_transition_weight = glm_smooth(texture_transition_weight);

	//////////

	UPDATE_UNIFORM(title_screen, shader, scrolling_light_pos_tangent_space, 3fv, 1, (vec3) {
		sinf(spin_seed), cosf(spin_seed),
		config -> shared.light_dist_from_screen_plane
	});

	UPDATE_UNIFORM(title_screen, shader, texture_transition_weight, 1f, texture_transition_weight);
	UPDATE_UNIFORM(title_screen, shader, scroll_factor, 1f, palace_city_hori_scroll);
}

////////// Initialization, deinitialization, and rendering

TitleScreen init_title_screen_from_json(const GLchar* const json_path) {
	cJSON JSON_OBJ_NAME_DEF(title_screen) = init_json_from_file(json_path);

	const cJSON
		DEF_JSON_SUBOBJ(title_screen, per_layer),
		DEF_JSON_SUBOBJ(title_screen, scrolling),
		DEF_JSON_SUBOBJ(title_screen, shared);

	const cJSON
		DEF_JSON_SUBOBJ(scrolling, bilinear_percents),
		DEF_JSON_SUBOBJ(scrolling, normal_map),
		DEF_JSON_SUBOBJ(shared, light_spin_cycle);

	const TitleScreenConfig title_screen_config = {
		// The `per_layer` field is set below

		.scrolling = {
			JSON_TO_FIELD(scrolling, vert_squish_ratio, float),

			.bilinear_percents = {
				JSON_TO_FIELD(bilinear_percents, albedo, float),
				JSON_TO_FIELD(bilinear_percents, normal, float),
			},

			.normal_map_config =  {
				JSON_TO_FIELD(normal_map, blur_radius, u8),
				JSON_TO_FIELD(normal_map, blur_std_dev, float),
				JSON_TO_FIELD(normal_map, heightmap_scale, float),
				JSON_TO_FIELD(normal_map, rescale_factor, float)
			},
		},

		.shared = {
			JSON_TO_FIELD(shared, texture_transition_immediacy_factor, u8),
			JSON_TO_FIELD(shared, tone_mapping_max_white, float),
			JSON_TO_FIELD(shared, noise_granularity, float),
			JSON_TO_FIELD(shared, light_dist_from_screen_plane, float),
			JSON_TO_FIELD(shared, secs_per_scroll_cycle, float),

			.light_spin_cycle = {
				JSON_TO_FIELD(light_spin_cycle, secs_per, float),
				JSON_TO_FIELD(light_spin_cycle, logo_transitions_per, float)
			}
		}
	};

	//////////

	validate_json_array(WITH_JSON_OBJ_SUFFIX(per_layer), num_title_screen_layers, num_title_screen_layers);

	JSON_FOR_EACH(i, layer_config, per_layer,
		DEF_ARRAY_FROM_JSON(layer_config, light_color, float, float, 3);
		DEF_ARRAY_FROM_JSON(layer_config, material_properties, float, float, 3);

		const TitleScreenPerLayerConfig this_layer_config = {
			JSON_TO_FIELD(layer_config, texture_path, string),
			JSON_TO_FIELD(layer_config, use_bilinear_filtering, bool),
			JSON_TO_FIELD(layer_config, ambient_strength, float),

			.light_color = {layer_config_light_color[0], layer_config_light_color[1], layer_config_light_color[2]},
			.material_properties = {layer_config_material_properties[0], layer_config_material_properties[1], layer_config_material_properties[2]}
		};

		memcpy((void*) (title_screen_config.per_layer + i), &this_layer_config, sizeof(TitleScreenPerLayerConfig));
	);

	//////////

	const TitleScreen title_screen = init_title_screen(&title_screen_config);
	deinit_json(WITH_JSON_OBJ_SUFFIX(title_screen));
	return title_screen;
}

TitleScreen init_title_screen(const TitleScreenConfig* const config) {
	const TextureType still_texture_type = TexPlain, scrolling_texture_type = TexSet;
	const TextureFilterMode min_filter = TexLinearMipmapped;

	////////// Getting the scrolling texture size

	const TitleScreenPerLayerConfig* const per_layer = config -> per_layer;
	const TitleScreenPerLayerConfig *const still_layer_config = per_layer, *const scrolling_layer_config = per_layer + 1;

	const GLchar* const scrolling_texture_path = scrolling_layer_config -> texture_path;
	SDL_Surface* const peek_surface = init_surface(scrolling_texture_path);
	const GLsizei scrolling_texture_size[2] = {peek_surface -> w, peek_surface -> h};
	deinit_surface(peek_surface);

	//////////

	/* This is only a texture set so that it can work with the shader function
	`get_albedo_and_normal` (TODO: genericize that one, if possible) */
	const GLuint scrolling_albedo_texture = init_texture_set(false, false, TexRepeating,
		scrolling_layer_config -> use_bilinear_filtering ? TexLinear : TexNearest,
		min_filter, 1, 0, scrolling_texture_size[0], scrolling_texture_size[1],
		(const GLchar*[]) {scrolling_texture_path}, NULL
	);

	// Overwriting the vertical wrap through a dumb hack
	glTexParameteri(scrolling_texture_type, GL_TEXTURE_WRAP_T, TexNonRepeating);

	const GLuint
		scrolling_normal_map = init_normal_map_from_albedo_texture(scrolling_albedo_texture,
			scrolling_texture_type, &config -> scrolling.normal_map_config),

		still_albedo_texture = init_plain_texture(still_layer_config -> texture_path, TexNonRepeating,
			still_layer_config -> use_bilinear_filtering ? TexLinear : TexNearest,
			min_filter, OPENGL_DEFAULT_INTERNAL_PIXEL_FORMAT);

	////////// Making a shader, and setting some uniforms

	const GLuint shader = init_shader("shaders/title_screen.vert", NULL, "shaders/title_screen.frag", NULL);
	use_shader(shader);

	INIT_UNIFORM_VALUE(scrolling_texture_vert_squish_ratio, shader, 1f, config -> scrolling.vert_squish_ratio);

	INIT_UNIFORM_VALUE(scrolling_bilinear_percents, shader, 2f,
		config -> scrolling.bilinear_percents.albedo,
		config -> scrolling.bilinear_percents.normal
	);

	INIT_UNIFORM_VALUE(tone_mapping_max_white, shader, 1f, config -> shared.tone_mapping_max_white);
	INIT_UNIFORM_VALUE(noise_granularity, shader, 1f, config -> shared.noise_granularity);

	////////// Setting some more uniforms

	vec3 still_and_scrolling_light_colors[num_title_screen_layers];
	vec4 still_and_scrolling_material_properties_and_ambient_strength[num_title_screen_layers];

	for (byte i = 0; i < num_title_screen_layers; i++) {
		const TitleScreenPerLayerConfig* const layer_config = per_layer + i;

		glm_vec3_copy((GLfloat*) layer_config -> light_color, still_and_scrolling_light_colors[i]);

		GLfloat* const material_properties_dest = still_and_scrolling_material_properties_and_ambient_strength[i];
		glm_vec3_copy((GLfloat*) layer_config -> material_properties, material_properties_dest);
		material_properties_dest[3] = layer_config -> ambient_strength;
	}

	////////// And setting some more

	INIT_UNIFORM_VALUE(still_and_scrolling_light_colors, shader, 3fv, num_title_screen_layers, (const GLfloat*) still_and_scrolling_light_colors);

	INIT_UNIFORM_VALUE(still_and_scrolling_material_properties_and_ambient_strength, shader, 4fv,
		num_title_screen_layers, (const GLfloat*) still_and_scrolling_material_properties_and_ambient_strength);

	////////// Setting a bunch of albedo samplers

	use_texture_in_shader(still_albedo_texture, shader, "still_albedo_sampler", still_texture_type, TU_TitleScreenStillAlbedo);
	use_texture_in_shader(scrolling_albedo_texture, shader, "scrolling_albedo_sampler", scrolling_texture_type, TU_TitleScreenScrollingAlbedo);
	use_texture_in_shader(scrolling_normal_map, shader, "scrolling_normal_sampler", scrolling_texture_type, TU_TitleScreenScrollingNormalMap);

	//////////

	return (TitleScreen) {
		.active = true,

		.drawable = init_drawable_without_vertices(
			(uniform_updater_t) update_uniforms,
			GL_TRIANGLE_STRIP, shader, scrolling_albedo_texture,
			scrolling_normal_map
		),

		.shader_uniform_ids = {
			INIT_UNIFORM_ID(scrolling_light_pos_tangent_space, shader),
			INIT_UNIFORM_ID(texture_transition_weight, shader),
			INIT_UNIFORM_ID(scroll_factor, shader)
		},

		.still_albedo_texture = still_albedo_texture,
		.config = *config
	};
}

void deinit_title_screen(const TitleScreen* const title_screen) {
	deinit_drawable(title_screen -> drawable);
	deinit_texture(title_screen -> still_albedo_texture);
}

bool tick_title_screen(TitleScreen* const title_screen, const Event* const event) {
	if (CHECK_BITMASK(event -> movement_bits, BIT_CLICK_LEFT))
		title_screen -> active = false;

	const bool active = title_screen -> active;

	if (active) {
		const UniformUpdaterParams uniform_updater_params = {title_screen, event -> curr_time_secs};
		draw_drawable(title_screen -> drawable, corners_per_quad, 0, &uniform_updater_params, UseShaderPipeline);
	}

	return active;
}
