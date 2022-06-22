#ifndef TITLE_SCREEN_C
#define TITLE_SCREEN_C

#include "headers/title_screen.h"
#include "headers/list.h"
#include "headers/shader.h"
#include "headers/texture.h"
#include "headers/constants.h"

static void update_uniforms(const Drawable* const drawable, const void* const param) {
	(void) param;

	static GLint brightness_id;

	ON_FIRST_CALL(
		const GLuint shader = drawable -> shader;
		INIT_UNIFORM(brightness, shader);
		use_texture(drawable -> diffuse_texture, shader, "texture_sampler", TexPlain, TITLE_SCREEN_LOGO_TEXTURE_UNIT);
	);

	const GLfloat
		m = constants.title_screen.min_brightness,
		time_seed = SDL_GetTicks() / constants.title_screen.brightness_repeat_ms;

	// This fluctuates between `m` and 1.0f
	const GLfloat brightness = (sinf(time_seed * PI) * 0.5f + 0.5f) * (1.0f - m) + m;
	UPDATE_UNIFORM(brightness, 1f, brightness);
}

TitleScreen init_title_screen(void) {
	return (TitleScreen) {
		.active = true,
		.drawable = init_drawable_without_vertices((uniform_updater_t) update_uniforms, GL_TRIANGLE_STRIP,
			init_shader(ASSET_PATH("shaders/title_screen.vert"), NULL, ASSET_PATH("shaders/title_screen.frag")),
			init_plain_texture(ASSET_PATH("logo.bmp"), TexPlain, TexNonRepeating, TexNearest, TexLinearMipmapped, OPENGL_DEFAULT_INTERNAL_PIXEL_FORMAT)
		)
	};
}

void deinit_title_screen(const TitleScreen* const title_screen) {
	deinit_drawable(title_screen -> drawable);
}

bool tick_title_screen(TitleScreen* const title_screen, const Event* const event) {
	if (CHECK_BITMASK(event -> movement_bits, BIT_CLICK_LEFT))
		title_screen -> active = false;

	const bool active = title_screen -> active;
	if (active) draw_drawable(title_screen -> drawable, corners_per_quad, NULL);
	return active;
}

#endif
