#ifndef INFO_BAR_H
#define INFO_BAR_H

#include "editor.h"

typedef struct {
	SDL_Rect area;
	TTF_Font* font;
	SDL_Texture* text_texture;
} InfoBar;

// Excluded: get_info_bar_string, update_info_bar_text

void init_info_bar(InfoBar* const info_bar);
void deinit_info_bar(const InfoBar* const info_bar);
void render_info_bar(InfoBar* const info_bar, const EditorState* const eds);

#endif
