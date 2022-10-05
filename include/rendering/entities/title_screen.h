#ifndef TITLE_SCREEN_H
#define TITLE_SCREEN_H

#include "utils/texture.h"
#include "normal_map_generation.h"
#include "utils/buffer_defs.h"
#include "rendering/drawable.h"
#include "event.h"

typedef struct {
	const struct {const GLchar *const still, *const scrolling;} paths;
	const struct {const TextureFilterMode still, scrolling;} mag_filters;
	const NormalMapConfig scrolling_normal_map_config;
} TitleScreenTextureConfig;

typedef struct {
	const byte texture_transition_immediacy_factor;

	const GLfloat
		scrolling_vert_squish_ratio,
		specular_exponent, scrolling_bilinear_percent,
		light_dist_from_screen_plane, secs_per_scroll_cycle;

	const struct {const GLfloat secs_per, logo_transitions_per;} light_spin_cycle;
} TitleScreenRenderingConfig;

//////////

typedef struct {
	bool active;
	const Drawable drawable;
	const GLuint still_diffuse_texture;
	const TitleScreenRenderingConfig rendering_config;
} TitleScreen;

// Excluded: update_uniforms

TitleScreen init_title_screen(
	const TitleScreenTextureConfig* const texture_config,
	const TitleScreenRenderingConfig* const rendering_config);

void deinit_title_screen(const TitleScreen* const title_screen);

// This returns if the title screen is active
bool tick_title_screen(TitleScreen* const title_screen, const Event* const event);

#endif
