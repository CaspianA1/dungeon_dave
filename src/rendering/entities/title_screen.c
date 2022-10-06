#include "rendering/entities/title_screen.h"
#include "utils/list.h"
#include "utils/shader.h"
#include "utils/opengl_wrappers.h"
#include "data/constants.h"

// TODO: let the mouse position equal or influence the light position

////////// Uniform updating

typedef struct {
	const TitleScreenRenderingConfig* const config;
	const GLfloat curr_time_secs;
} UniformUpdaterParams;

static void update_uniforms(const Drawable* const drawable, const void* const param) {
	const UniformUpdaterParams typed_params = *(UniformUpdaterParams*) param;

	static GLint light_pos_tangent_space_id, texture_transition_weight_id, scroll_factor_id;
	static GLfloat base_time_secs;

	ON_FIRST_CALL(
		const GLuint shader = drawable -> shader;

		INIT_UNIFORM(light_pos_tangent_space, shader);
		INIT_UNIFORM(texture_transition_weight, shader);
		INIT_UNIFORM(scroll_factor, shader);

		base_time_secs = typed_params.curr_time_secs;
	);

	const GLfloat relative_time_secs = typed_params.curr_time_secs - base_time_secs;

	const GLfloat
		palace_city_hori_scroll = relative_time_secs / typed_params.config -> secs_per_scroll_cycle,
		spin_seed = relative_time_secs * TWO_PI / typed_params.config -> light_spin_cycle.secs_per;

	GLfloat texture_transition_weight = cosf(spin_seed * typed_params.config -> light_spin_cycle.logo_transitions_per) * 0.5f + 0.5f;
	for (byte i = 0; i < typed_params.config -> texture_transition_immediacy_factor; i++) texture_transition_weight = glm_smooth(texture_transition_weight);

	UPDATE_UNIFORM(light_pos_tangent_space, 3fv, 1, (vec3) {sinf(spin_seed), cosf(spin_seed), typed_params.config -> light_dist_from_screen_plane});
	UPDATE_UNIFORM(texture_transition_weight, 1f, texture_transition_weight);
	UPDATE_UNIFORM(scroll_factor, 1f, palace_city_hori_scroll);
}

////////// Initialization, deinitialization, and rendering

TitleScreen init_title_screen(
	const TitleScreenTextureConfig* const texture_config,
	const TitleScreenRenderingConfig* const rendering_config) {

	const TextureType texture_type = TexPlain;

	const GLuint scrolling_diffuse_texture = init_plain_texture(texture_config -> paths.scrolling, texture_type,
		TexRepeating, texture_config -> mag_filters.scrolling, TexLinearMipmapped, OPENGL_DEFAULT_INTERNAL_PIXEL_FORMAT);

	// Overwriting the vertical wrap through a dumb hack
	glTexParameteri(texture_type, GL_TEXTURE_WRAP_T, TexNonRepeating);

	const GLuint
		scrolling_normal_map = init_normal_map_from_diffuse_texture(scrolling_diffuse_texture,
			texture_type, &texture_config -> scrolling_normal_map_config),

		still_diffuse_texture = init_plain_texture(texture_config -> paths.still, texture_type, TexNonRepeating,
				texture_config -> mag_filters.still, TexLinearMipmapped, OPENGL_DEFAULT_INTERNAL_PIXEL_FORMAT);

	//////////

	const GLuint shader = init_shader(ASSET_PATH("shaders/title_screen.vert"), NULL, ASSET_PATH("shaders/title_screen.frag"), NULL);
	use_shader(shader);

	INIT_UNIFORM_VALUE(scrolling_texture_vert_squish_ratio, shader, 1f, rendering_config -> scrolling_vert_squish_ratio);
	INIT_UNIFORM_VALUE(specular_exponent, shader, 1f, rendering_config -> specular_exponent);
	INIT_UNIFORM_VALUE(scrolling_bilinear_diffuse_percent, shader, 1f, rendering_config -> scrolling_bilinear_diffuse_percent);
	INIT_UNIFORM_VALUE(scrolling_bilinear_normal_percent, shader, 1f, rendering_config -> scrolling_bilinear_normal_percent);

	use_texture_in_shader(still_diffuse_texture, shader, "still_diffuse_sampler", TexPlain, TU_TitleScreenStillDiffuse);
	use_texture_in_shader(scrolling_diffuse_texture, shader, "scrolling_diffuse_sampler", TexPlain, TU_TitleScreenScrollingDiffuse);
	use_texture_in_shader(scrolling_normal_map, shader, "scrolling_normal_map_sampler", TexPlain, TU_TitleScreenScrollingNormalMap);

	//////////

	return (TitleScreen) {
		.active = true,

		.drawable = init_drawable_without_vertices(
			(uniform_updater_t) update_uniforms,
			GL_TRIANGLE_STRIP, shader, scrolling_diffuse_texture,
			scrolling_normal_map
		),

		.still_diffuse_texture = still_diffuse_texture,

		.rendering_config = *rendering_config
	};
}

void deinit_title_screen(const TitleScreen* const title_screen) {
	deinit_drawable(title_screen -> drawable);
	deinit_texture(title_screen -> still_diffuse_texture);
}

bool tick_title_screen(TitleScreen* const title_screen, const Event* const event) {
	if (CHECK_BITMASK(event -> movement_bits, BIT_CLICK_LEFT))
		title_screen -> active = false;

	const bool active = title_screen -> active;

	if (active) {
		const UniformUpdaterParams uniform_updater_params = {&title_screen -> rendering_config, event -> curr_time_secs};
		draw_drawable(title_screen -> drawable, corners_per_quad, 0, &uniform_updater_params, UseShaderPipeline);
	}

	return active;
}
