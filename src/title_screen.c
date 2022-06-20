#ifndef TITLE_SCREEN_C
#define TITLE_SCREEN_C

#include "headers/title_screen.h"
#include "headers/shader.h"
#include "headers/texture.h"
#include "headers/constants.h"

// TODO: use Drawable for this

TitleScreen init_title_screen(void) {
	return (TitleScreen) {
		.done = false,
		.shader = init_shader(ASSET_PATH("shaders/title_screen.vert"), NULL, ASSET_PATH("shaders/title_screen.frag")),
		.texture = init_plain_texture(ASSET_PATH("logo.bmp"), TexPlain, TexNonRepeating,
			TexNearest, TexLinearMipmapped, OPENGL_DEFAULT_INTERNAL_PIXEL_FORMAT)
	};
}

bool title_screen_finished(TitleScreen* const title_screen, const Event* const event) {
	if (CHECK_BITMASK(event -> movement_bits, BIT_CLICK_LEFT))
		title_screen -> done = true;

	return title_screen -> done;
}

void tick_title_screen(const TitleScreen title_screen) {
	use_shader(title_screen.shader);

	static GLint brightness_id;

	ON_FIRST_CALL(
		INIT_UNIFORM(brightness, title_screen.shader);
		use_texture(title_screen.texture, title_screen.shader, "texture_sampler", TexPlain, TITLE_SCREEN_LOGO_TEXTURE_UNIT);
	);

	const GLfloat
		m = constants.title_screen.min_brightness,
		time_seed = SDL_GetTicks() / constants.title_screen.brightness_repeat_ms;

	// This fluctuates between `m` and 1.0f
	const GLfloat brightness = (sinf(time_seed * PI) * 0.5f + 0.5f) * (1.0f - m) + m;
	UPDATE_UNIFORM(brightness, 1f, brightness);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, corners_per_quad);
}

void deinit_title_screen(const TitleScreen* const title_screen) {
	deinit_shader(title_screen -> shader);
	deinit_texture(title_screen -> texture);
}

#endif
