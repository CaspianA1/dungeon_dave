#ifndef INFO_BAR_H
#define INFO_BAR_H

#include "editor.h"

typedef struct {
	SDL_Rect area;
	char* text;
	TTF_Font* font;
	SDL_Texture* text_texture;
} InfoBar;

// Excluded: update_info_bar_text, update_info_bar_texture

void init_info_bar(InfoBar* const info_bar);
void deinit_info_bar(const InfoBar* const info_bar);
void render_info_bar(InfoBar* const info_bar, const EditorState* const eds);

#endif
