#ifndef INFO_BAR_H
#define INFO_BAR_H

#include "editor.h"

typedef struct {
	char* text;
	SDL_Texture *font_texture, *background_texture;
	int font_texture_size[2];
} InfoBar;

// Excluded: update_info_bar_text, update_info_bar_texture

void init_info_bar(InfoBar* const info_bar, SDL_Renderer* const renderer);
void deinit_info_bar(const InfoBar* const info_bar);
void render_info_bar(InfoBar* const info_bar, const EditorState* const eds);

#define FONT_PATH "../assets/dungeon_font.bmp"
#define INFO_BAR_TEXTURE_PATH "../../../assets/skyboxes/palace_city.bmp"
#define INFO_BAR_COLOR 0, 255, 127, 255

#endif
