#include "rendering/entities/title_screen.h"
#include "utils/list.h"
#include "utils/shader.h"
#include "utils/texture.h"
#include "normal_map_generation.h"
#include "data/constants.h"

// TODO: incorporate the palace city texture into this in some way

////////// Uniform updating

typedef struct {
	const TitleScreen* const title_screen;
	const GLfloat curr_time_secs;
} UniformUpdaterParams;

static void update_uniforms(const Drawable* const drawable, const void* const param) {
	const UniformUpdaterParams typed_params = *(UniformUpdaterParams*) param;
	const GLuint shader = drawable -> shader;

	static GLint light_pos_tangent_space_id, texture_transition_weight_id, palace_city_hori_scroll_id;

	ON_FIRST_CALL(
		INIT_UNIFORM(light_pos_tangent_space, shader);
		INIT_UNIFORM(texture_transition_weight, shader);
		INIT_UNIFORM(palace_city_hori_scroll, shader);

		INIT_UNIFORM_VALUE(specular_exponent, shader, 1f, 8.0f);
		INIT_UNIFORM_VALUE(palace_city_vert_squish_ratio, shader, 1f, 0.35f);

		use_texture(typed_params.title_screen -> logo_diffuse_texture, shader, "logo_diffuse_sampler", TexPlain, TU_TitleScreenLogoDiffuse);
		use_texture(drawable -> diffuse_texture, shader, "palace_city_diffuse_sampler", TexPlain, TU_TitleScreenPalaceCityDiffuse);
		use_texture(typed_params.title_screen -> palace_city_normal_map, shader, "palace_city_normal_map_sampler", TexPlain, TU_TitleScreenPalaceCityNormalMap);
	);

	const GLfloat // TODO: put these in the `constants` struct
		time_for_spin_cycle = 3.0f, logo_transitions_per_spin_cycle = 0.25f,
		light_dist_from_title_screen_plane = 0.75f, time_for_palace_city_scroll_cycle = 10.0f;

	const GLfloat spin_seed = typed_params.curr_time_secs * TWO_PI / time_for_spin_cycle;
	const GLfloat texture_transition_weight = cosf(spin_seed * logo_transitions_per_spin_cycle) * 0.5f + 0.5f;
	const GLfloat palace_city_hori_scroll = typed_params.curr_time_secs / time_for_palace_city_scroll_cycle;

	UPDATE_UNIFORM(light_pos_tangent_space, 3fv, 1, (vec3) {sinf(spin_seed), cosf(spin_seed), light_dist_from_title_screen_plane});
	UPDATE_UNIFORM(texture_transition_weight, 1f, texture_transition_weight);
	UPDATE_UNIFORM(palace_city_hori_scroll, 1f, palace_city_hori_scroll);
}

////////// Initialization, deinitialization, and rendering

TitleScreen init_title_screen(void) {
	const GLuint palace_city_texture = init_plain_texture(ASSET_PATH("palace_city.bmp"), TexPlain,
		TexRepeating, TexLinear, TexLinearMipmapped, OPENGL_DEFAULT_INTERNAL_PIXEL_FORMAT
	);

	// Overwriting the vertical wrap through a dumb hack
	glTexParameteri(TexPlain, GL_TEXTURE_WRAP_T, TexNonRepeating);

	//////////

	// TODO: make this a constant or parameter somewhere
	const NormalMapConfig normal_map_config = {.blur_radius = 8, .blur_std_dev = 1.5f, .intensity = 1.0f, .rescale_factor = 1.0f};

	return (TitleScreen) {
		.active = true,
		.drawable = init_drawable_without_vertices((uniform_updater_t) update_uniforms, GL_TRIANGLE_STRIP,
			init_shader(ASSET_PATH("shaders/title_screen.vert"), NULL, ASSET_PATH("shaders/title_screen.frag")),
			palace_city_texture
		),

		.palace_city_normal_map = init_normal_map_from_diffuse_texture(palace_city_texture, TexPlain, &normal_map_config),

		.logo_diffuse_texture = init_plain_texture(ASSET_PATH("logo.bmp"), TexPlain,
			TexNonRepeating, TexNearest, TexLinearMipmapped, OPENGL_DEFAULT_INTERNAL_PIXEL_FORMAT
		)
	};
}

void deinit_title_screen(const TitleScreen* const title_screen) {
	deinit_drawable(title_screen -> drawable);
	glDeleteTextures(2, (GLuint[2]) {title_screen -> palace_city_normal_map, title_screen -> logo_diffuse_texture});
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
