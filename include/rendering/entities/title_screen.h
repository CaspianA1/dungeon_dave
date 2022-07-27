#ifndef TITLE_SCREEN_H
#define TITLE_SCREEN_H

#include "utils/buffer_defs.h"
#include "rendering/drawable.h"
#include "event.h"

typedef struct {
	bool active;
	const Drawable drawable;
} TitleScreen;

// Excluded: update_uniforms

TitleScreen init_title_screen(const GLchar* const title_screen_path);
void deinit_title_screen(const TitleScreen* const title_screen);

// This returns if the title screen is active
bool tick_title_screen(TitleScreen* const title_screen, const Event* const event);

#endif
