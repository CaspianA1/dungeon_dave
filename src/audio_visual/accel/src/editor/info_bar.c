#include "info_bar.h"

/*
- Some background texture
- Keep track of state through a LastEditorState struct, to determine if need to change ui

Format of info bar: | name | tile pos | editing mode + editing val | move, draw, or erase |

Examples:
palace | pos 3, 15 | height 8 | move
fleckenstein | 4, 20 | tex 19 | erase

- Highlight tex or height if editing it
- Make font letters equal sizes - or pad numbers with spaces (to a fixed width font) (glyphrstudio)
- Padding with spaces should work, since the padding should be the max space able to be taken
- Check DEBUG(TTF_FontFaceIsFixedWidth(info_bar.font, d));
- Get edit state from eds (move, draw, or erase) - done
- A 'select texture' or 'select height' button
- Remember to free the info bar, and only reallocate it when needed
- Or maybe a completely new font, that's monospace from the beginning (only lowercase + numbers)
*/

static char* get_info_bar_string(const EditorState* const eds, const char* const map_name) {
	const char *edit_state, *edit_mode;
	byte edit_mode_val;
	const byte map_x = eds -> tile_pos[0], map_y = eds -> tile_pos[1];

	switch (eds -> mouse_state) {
		case LeftClick: edit_state = "draw"; break;
		case RightClick: edit_state = "erase"; break;
		default: edit_state = "move"; break;
	}

	if (eds -> in_texture_editing_mode) {
		edit_mode = "texture";
		edit_mode_val = eds -> editing_texture_id;
	}
	else {
		edit_mode = "height";
		edit_mode_val = eds -> editing_height;
	}

	//////////

	/* Tile pos: max 3 chars b/c max = 255. Editing mode: max 7 chars b/c "texture" is 7 chars. Editing
	mode value: max 2 chars b/c max texture id is 31. Editing state: max 5 chars b/c "erase" is 5 chars. */
	const char* const info_bar_format = "| %s | x %-3d | y %-3d | %-7s #%-2d | %-5s |";

	const size_t bytes = snprintf(NULL, 0, info_bar_format,
		map_name, map_x, map_y, edit_mode, edit_mode_val, edit_state);

	char* const info_bar_string = malloc(bytes + 1);
	sprintf(info_bar_string, info_bar_format,
		map_name, map_x, map_y, edit_mode, edit_mode_val, edit_state);
	
	return info_bar_string;
}

static void update_info_bar_text(const EditorState* const eds, InfoBar* const info_bar, const char* const map_name) {
	char* const text = get_info_bar_string(eds, map_name);

	SDL_Surface* const text_surface = TTF_RenderText_Solid(info_bar -> font, text, (SDL_Color) {255, 0, 0, 0});
	if (text_surface == NULL)
		FAIL(CreateTextSurface, "Could not create a text surface: \"%s\"\n", TTF_GetError());

	SDL_Texture* text_texture = info_bar -> text_texture;
	if (text_texture != NULL) SDL_DestroyTexture(text_texture); // Destroying the last texture

	text_texture = SDL_CreateTextureFromSurface(eds -> renderer, text_surface);
	if (text_texture == NULL)
		FAIL(CreateTexture, "Could not create a text texture: \"%s\"", SDL_GetError());

	info_bar -> text_texture = text_texture;
	SDL_FreeSurface(text_surface);
	free(text);
}

void init_info_bar(InfoBar* const info_bar) {
	info_bar -> text_texture = NULL;

	const int info_bar_height = EDITOR_HEIGHT - EDITOR_MAP_SECTION_HEIGHT;
	info_bar -> area = (SDL_Rect) {0, EDITOR_MAP_SECTION_HEIGHT, EDITOR_WIDTH, info_bar_height};

	info_bar -> font = TTF_OpenFont(FONT_PATH, info_bar_height);
	if (info_bar -> font == NULL)
		FAIL(OpenFile, "Could not open the UI font file: \"%s\".", TTF_GetError());
}

void deinit_info_bar(const InfoBar* const info_bar) {
	SDL_DestroyTexture(info_bar -> text_texture);
	TTF_CloseFont(info_bar -> font);
}

void render_info_bar(InfoBar* const info_bar, const EditorState* const eds) {
	update_info_bar_text(eds, info_bar, eds -> map_name);
	SDL_RenderCopy(eds -> renderer, info_bar -> text_texture, NULL, &info_bar -> area);
}
