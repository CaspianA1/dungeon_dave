#ifndef TITLE_SCREEN_H
#define TITLE_SCREEN_H

#include "buffer_defs.h"
#include "event.h"

typedef struct {
	bool done;
	GLuint shader, texture;
} TitleScreen;

TitleScreen init_title_screen(void);
bool title_screen_finished(TitleScreen* const title_screen, const Event* const event);
void tick_title_screen(const TitleScreen title_screen);
void deinit_title_screen(const TitleScreen* const title_screen);

#endif
