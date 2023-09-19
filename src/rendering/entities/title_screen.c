#include "rendering/entities/title_screen.h"
#include "utils/macro_utils.h" // For `ON_FIRST_CALL`
#include "utils/opengl_wrappers.h" // For `INIT_UNIFORM`, and `UPDATE_UNIFORM`
#include "data/constants.h" // For `TWO_PI`
#include "utils/safe_io.h" // For `get_temp_asset_path`
#include "utils/shader.h" // For `init_shader`

////////// Uniform updating

typedef struct {
	const TitleScreenConfig* const config;
	const GLfloat curr_time_secs;
} UniformUpdaterParams;

static void update_uniforms(const Drawable* const drawable, const void* const param) {
	const UniformUpdaterParams typed_params = *(UniformUpdaterParams*) param;

	static GLint scrolling_light_pos_tangent_space_id, texture_transition_weight_id, scroll_factor_id;
	static GLfloat base_time_secs;

	ON_FIRST_CALL(
		const GLuint shader = drawable -> shader;

		INIT_UNIFORM(scrolling_light_pos_tangent_space, shader);
		INIT_UNIFORM(texture_transition_weight, shader);
		INIT_UNIFORM(scroll_factor, shader);

		base_time_secs = typed_params.curr_time_secs;
	);

	//////////

	const GLfloat relative_time_secs = typed_params.curr_time_secs - base_time_secs;

	const GLfloat // Modding the hori scroll with 1 to avoid floating-point imprecision errors for large time values
		palace_city_hori_scroll = fmodf(relative_time_secs / typed_params.config -> shared.secs_per_scroll_cycle, 1.0f),
		spin_seed = relative_time_secs * TWO_PI / typed_params.config -> shared.light_spin_cycle.secs_per;

	/* TODO: if used, make this focus on different quadrants each time
	(and make sure that `secs_per` is still followed) */
	// spin_seed += tanf(sinf(spin_seed));

	GLfloat texture_transition_weight = cosf(
		spin_seed * typed_params.config -> shared.light_spin_cycle.logo_transitions_per
	) * 0.5f + 0.5f;

	for (byte i = 0; i < typed_params.config -> shared.texture_transition_immediacy_factor; i++)
		texture_transition_weight = glm_smooth(texture_transition_weight);

	//////////

	UPDATE_UNIFORM(scrolling_light_pos_tangent_space, 3fv, 1, (vec3) {
		sinf(spin_seed), cosf(spin_seed),
		typed_params.config -> shared.light_dist_from_screen_plane
	});

	UPDATE_UNIFORM(texture_transition_weight, 1f, texture_transition_weight);
	UPDATE_UNIFORM(scroll_factor, 1f, palace_city_hori_scroll);
}

////////// Initialization, deinitialization, and rendering

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
	const GLuint scrolling_albedo_texture = init_texture_set(false, TexRepeating,
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

	//////////

	const GLuint shader = init_shader("shaders/title_screen.vert", NULL, "shaders/title_screen.frag", NULL);
	use_shader(shader);

	INIT_UNIFORM_VALUE(scrolling_texture_vert_squish_ratio, shader, 1f, config -> scrolling.vert_squish_ratio);

	INIT_UNIFORM_VALUE(scrolling_bilinear_percents, shader, 2f,
		config -> scrolling.bilinear_percents.albedo,
		config -> scrolling.bilinear_percents.normal
	);

	INIT_UNIFORM_VALUE(tone_mapping_max_white, shader, 1f, config -> shared.tone_mapping_max_white);
	INIT_UNIFORM_VALUE(noise_granularity, shader, 1f, config -> shared.noise_granularity);

	//////////

	enum {num_layers = ARRAY_LENGTH(config -> per_layer)};

	vec3 still_and_scrolling_light_colors[num_layers];
	vec4 still_and_scrolling_material_properties_and_ambient_strength[num_layers];

	for (byte i = 0; i < num_layers; i++) {
		const TitleScreenPerLayerConfig* const layer_config = per_layer + i;

		glm_vec3_copy((GLfloat*) layer_config -> light_color, still_and_scrolling_light_colors[i]);

		GLfloat* const material_properties_dest = still_and_scrolling_material_properties_and_ambient_strength[i];
		glm_vec3_copy((GLfloat*) layer_config -> material_properties, material_properties_dest);
		material_properties_dest[3] = layer_config -> ambient_strength;
	}

	//////////

	INIT_UNIFORM_VALUE(still_and_scrolling_light_colors, shader, 3fv, num_layers,
		(const GLfloat*) still_and_scrolling_light_colors);

	INIT_UNIFORM_VALUE(still_and_scrolling_material_properties_and_ambient_strength, shader, 4fv, num_layers,
		(const GLfloat*) still_and_scrolling_material_properties_and_ambient_strength);

	//////////

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
		const UniformUpdaterParams uniform_updater_params = {&title_screen -> config, event -> curr_time_secs};
		draw_drawable(title_screen -> drawable, corners_per_quad, 0, &uniform_updater_params, UseShaderPipeline);
	}

	return active;
}
