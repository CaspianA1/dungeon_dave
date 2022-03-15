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
- Perhaps make the right click not erase, but something more useful
*/

static void update_info_bar_text(const EditorState* const eds,
	char** const text_buffer_ref, const char* const map_name) {

	const bool editing_texture = eds -> in_texture_editing_mode;
	const byte map_x = eds -> tile_pos[0], map_y = eds -> tile_pos[1];
	const byte editor_placement_val = editing_texture ? eds -> editor_texture_id : eds -> editor_height;

	static const char
		*const edit_state_strings[3] = {"draw", "erase", "move"},
		*const edit_mode_strings[2] = {"height", "texture"},
		*const info_bar_format = "|%s|x %-3d|y %-3d|%7s %-3d|%-5s|";

		/* Tile pos: max 3 chars b/c max = 255. Editing mode: max 7 chars b/c "texture" is 7 chars. Editor
		placement value: max 3 chars b/c for the two possible value types, texture and height, the max height val
		is 255. Editing state: max 5 chars b/c "erase" is 5 chars. */

	const char
		*const edit_state = edit_state_strings[eds -> mouse_state],
		*const edit_mode = edit_mode_strings[editing_texture];

	//////////

	char* text_buffer = *text_buffer_ref;

	if (text_buffer == NULL) {
		const int num_bytes = snprintf(NULL, 0, info_bar_format,
			map_name, map_x, map_y, edit_mode, editor_placement_val, edit_state);

		text_buffer = malloc((size_t) (num_bytes + 1));
	}

	sprintf(text_buffer, info_bar_format,
		map_name, map_x, map_y, edit_mode, editor_placement_val, edit_state);

	*text_buffer_ref = text_buffer;
}

void init_info_bar(InfoBar* const info_bar, SDL_Renderer* const renderer) {
	info_bar -> text = NULL;
	info_bar -> font_texture = init_texture(FONT_PATH, renderer, 0);
	info_bar -> background_texture = init_texture(INFO_BAR_TEXTURE_PATH, renderer, 1);

	SDL_QueryTexture(info_bar -> font_texture, NULL, NULL,
		info_bar -> font_texture_size, info_bar -> font_texture_size + 1);
}

void deinit_info_bar(const InfoBar* const info_bar) {
	free(info_bar -> text);
	SDL_DestroyTexture(info_bar -> font_texture);
}

void render_info_bar(InfoBar* const info_bar, const EditorState* const eds) {
	SDL_Renderer* const renderer = eds -> renderer;
	const SDL_Rect info_bar_area = {0, EDITOR_MAP_SECTION_HEIGHT, EDITOR_WIDTH, EDITOR_HEIGHT - EDITOR_MAP_SECTION_HEIGHT};
	SDL_RenderCopy(renderer, info_bar -> background_texture, NULL, &info_bar_area);

	//////////

	update_info_bar_text(eds, &info_bar -> text, eds -> map_name);

	const int
		glyph_src_width = info_bar -> font_texture_size[0] / FONT_GLYPH_COUNT,
		glyph_src_height = info_bar -> font_texture_size[1],
		glyph_dest_width = info_bar_area.w / (int) strlen(info_bar -> text);

	for (const char* c_ref = info_bar -> text; *c_ref != '\0'; c_ref++) {
		const char c = *c_ref;

		char letter_index;
		if (c >= 'a' && c <= 'z') letter_index = c - 'a';
		else if (c >= '0' && c <= '9') letter_index = c - '0' + 26;
		else if (c == '|') letter_index = 36;
		else continue;

		const SDL_Rect
			src_crop = {
				letter_index * glyph_src_width, 0,
				glyph_src_width, glyph_src_height
			},
			dest_crop = {
				(int) (c_ref - info_bar -> text) * glyph_dest_width, EDITOR_MAP_SECTION_HEIGHT,
				glyph_dest_width, info_bar_area.h
			};

		SDL_RenderCopy(renderer, info_bar -> font_texture, &src_crop, &dest_crop);
	}
}
