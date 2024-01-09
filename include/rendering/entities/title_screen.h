#ifndef TITLE_SCREEN_H
#define TITLE_SCREEN_H

#include "data/constants.h" // For `num_title_screen_layers`
#include "glad/glad.h" // For OpenGL defs
#include "utils/typedefs.h" // For various typedefs
#include "utils/texture.h" // For `TextureFilterMode`
#include "utils/normal_map_generation.h" // For `NormalMapConfig`
#include <stdbool.h> // For `bool`
#include "rendering/drawable.h" // For `Drawable`
#include "event.h" // For `Event`

typedef struct {
	const GLchar* const texture_path;
	const bool use_bilinear_filtering;
	const GLfloat ambient_strength;
	const vec3 light_color, material_properties;
} TitleScreenPerLayerConfig;

typedef struct {
	const TitleScreenPerLayerConfig per_layer[num_title_screen_layers];

	const struct {
		const GLfloat vert_squish_ratio;
		const struct {const GLfloat albedo, normal;} bilinear_percents;
		const NormalMapConfig normal_map_config;
	} scrolling;

	const struct {
		const byte texture_transition_immediacy_factor;

		const GLfloat
			tone_mapping_max_white, noise_granularity,
			light_dist_from_screen_plane, secs_per_scroll_cycle;

		const struct {
			const GLfloat secs_per, logo_transitions_per;
		} light_spin_cycle;
	} shared;
} TitleScreenConfig;

typedef struct {
	bool active;
	const Drawable drawable;

	const struct {
		const GLint
			scrolling_light_pos_tangent_space,
			texture_transition_weight,
			scroll_factor;
	} shader_uniform_ids;

	const GLuint still_albedo_texture;
	const TitleScreenConfig config;
} TitleScreen;

// Excluded: update_uniforms

// TODO: make other functions like this per each key game object
TitleScreen init_title_screen_from_json(const GLchar* const json_path);
TitleScreen init_title_screen(const TitleScreenConfig* const config);
void deinit_title_screen(const TitleScreen* const title_screen);

// This returns if the title screen is active
bool tick_title_screen(TitleScreen* const title_screen, const Event* const event);

#endif
