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

	static GLint texture_transition_weight_id, light_pos_tangent_space_id;

	ON_FIRST_CALL(
		INIT_UNIFORM(texture_transition_weight, shader);
		INIT_UNIFORM(light_pos_tangent_space, shader);
		use_texture(typed_params.title_screen -> logo_diffuse_texture, shader, "logo_diffuse_sampler", TexPlain, TU_TitleScreenLogoDiffuse);
		use_texture(drawable -> diffuse_texture, shader, "palace_city_diffuse_sampler", TexPlain, TU_TitleScreenPalaceCityDiffuse);
		use_texture(typed_params.title_screen -> palace_city_normal_map, shader, "palace_city_normal_map_sampler", TexPlain, TU_TitleScreenPalaceCityNormalMap);
	);

	// TODO: put these in the `constants` struct
	const GLfloat time_for_spin_cycle = 2.0f, logo_transitions_per_spin_cycle = 0.25f, light_dist_from_title_screen_plane = 0.75f;

	const GLfloat spin_seed = typed_params.curr_time_secs * TWO_PI / time_for_spin_cycle;
	const GLfloat texture_transition_weight = cosf(spin_seed * logo_transitions_per_spin_cycle) * 0.5f + 0.5f;

	UPDATE_UNIFORM(light_pos_tangent_space, 3fv, 1, (vec3) {sinf(spin_seed), cosf(spin_seed), light_dist_from_title_screen_plane});
	UPDATE_UNIFORM(texture_transition_weight, 1f, texture_transition_weight);
}

////////// Initialization, deinitialization, and rendering

TitleScreen init_title_screen(void) {
	const GLuint texture = init_plain_texture(ASSET_PATH("palace_city.bmp"), TexPlain,
		TexNonRepeating, TexLinear, TexLinearMipmapped, OPENGL_DEFAULT_INTERNAL_PIXEL_FORMAT
	);

	// TODO: make this a constant or parameter somewhere
	const NormalMapConfig normal_map_config = {.blur_radius = 8, .blur_std_dev = 1.5f, .intensity = 1.0f};

	return (TitleScreen) {
		.active = true,
		.drawable = init_drawable_without_vertices((uniform_updater_t) update_uniforms, GL_TRIANGLE_STRIP,
			init_shader(ASSET_PATH("shaders/title_screen.vert"), NULL, ASSET_PATH("shaders/title_screen.frag")),
			texture
		),

		.palace_city_normal_map = init_normal_map_from_diffuse_texture(texture, TexPlain, &normal_map_config),

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