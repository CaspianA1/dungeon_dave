#ifndef TITLE_SCREEN_H
#define TITLE_SCREEN_H

#include "utils/buffer_defs.h"
#include "rendering/drawable.h"
#include "event.h"

typedef struct {
	bool active;
	const Drawable drawable;
	const GLuint palace_city_normal_map, logo_diffuse_texture;
} TitleScreen;

// Excluded: update_uniforms

TitleScreen init_title_screen(void);
void deinit_title_screen(const TitleScreen* const title_screen);

// This returns if the title screen is active
bool tick_title_screen(TitleScreen* const title_screen, const Event* const event);

#endif
