#include "editor.h"
#include "info_bar.c"
#include "level_parser.c"
#include "../data/maps.c"

/*
Plan:

- A sample-map button
- Read in level files
- Verify that maps in level files are the right dimensions
- Report repeat sections in input files
- An 'undo previous action' feature (undoes to the action done before the mouse was clicked down and then released)
- Later on, line and rectangle functions (or maybe just a line function)
- Sometimes, clicking for too long freezes my computer
- 2 options: start new, and edit current. Begin with start new.
- A tag for music

- An info bar - done
- Show height from shading - darker = higher; and a max height as an input too - done
- Drag and click to draw to a map - done
- Right click while dragging to erase - done
- Press <esc> to toggle texture editing mode - done
- Input a series of numbers, and then hit return (stopping if more than 3 numbers), to select a height or texture id number - done
- If number over 255, limit to that - done
- Variables editor_height and editor_texture_id - done
- Display point height over each texture in some good way - done
- Selected block highlighted - done
- Textures uneven if using SDL_RenderCopyF - done and fixed
- A bottom bar that gives the current texture and current height - done
- Press a number key to change the current tex - done
- Info bar background - done
- Occasional info bar crashes - done
- For the parser, a byte that tells which sections have been parsed (for detecting sections repeated, missing, and map_size after maps) - done
*/

byte* map_point(const EditorState* const eds, const byte is_heightmap, const byte x, const byte y) {
	byte* const map = is_heightmap ? eds -> heightmap : eds -> texture_id_map;
	return map + (y * eds -> map_size[0] + x);
}

SDL_Texture* init_texture(const char* const path, SDL_Renderer* const renderer, const byte linear_filtering) {
	SDL_SetHintWithPriority(SDL_HINT_RENDER_SCALE_QUALITY, linear_filtering ? "1" : "0", SDL_HINT_OVERRIDE);

	SDL_Surface* const surface = SDL_LoadBMP(path);
	if (surface == NULL) FAIL(OpenFile, "Surface with path '%s' not found", path);

	SDL_Texture* const texture = SDL_CreateTextureFromSurface(renderer, surface);
	if (texture == NULL) FAIL(CreateTexture, "Could not create a wall texture: \"%s\"", SDL_GetError());

	SDL_FreeSurface(surface);
	return texture;
}

int16_t three_chars_to_int(const char chars[3], const byte num_chars_to_convert) {
	int16_t number = 0;

	for (byte i = 0; i < num_chars_to_convert; i++)
		number = number * 10 + (chars[i] - '0');

	return number;
}

// Updates editor_height and editor_texture_id. Reads in 3 nums across function calls to update one
static void update_editing_placement_values(EditorState* const eds, const SDL_Event* const event) {
	static char number_input_chars[3];
	const SDL_Keycode key = event -> key.keysym.sym;

	static byte num_chars_inputted = 0;
	byte number_input_done = 0;

	if (key == SDLK_RETURN && num_chars_inputted != 0) number_input_done = 1;
	else if (key >= SDLK_0 && key <= SDLK_9) {
		number_input_chars[num_chars_inputted] = key;
		if (++num_chars_inputted == 3) number_input_done = 1;
	}
	else num_chars_inputted = 0;

	if (number_input_done) {
		int16_t number = three_chars_to_int(number_input_chars, num_chars_inputted);
		if (number > 255) number = 255;

		num_chars_inputted = 0;

		const byte max_texture_id = eds -> num_textures - 1; // Avoiding too big of a texture id
		if (eds -> in_texture_editing_mode) number = (number > max_texture_id) ? max_texture_id : number;

		*(eds -> in_texture_editing_mode ? &eds -> editor_texture_id : &eds -> editor_height) = number;
	}
}

static void edit_eds_map(EditorState* const eds) {
	static byte prev_texture_edit_key = 0, first_call = 1;
	static const Uint8* keys;

	if (first_call) {
		keys = SDL_GetKeyboardState(NULL);
		first_call = 0;
	}

	const byte texture_edit_key = keys[KEY_TOGGLE_TEXTURE_EDIT_MODE];
	if (texture_edit_key)
		if (!prev_texture_edit_key) eds -> in_texture_editing_mode = !eds -> in_texture_editing_mode;
	prev_texture_edit_key = texture_edit_key;

	//////////

	// Make click based on if mouse still down, not just down in one instance
	if (eds -> mouse_state != NoClick && eds -> mouse_pos[1] < EDITOR_MAP_SECTION_HEIGHT) {
		byte map_output_val;

		if (eds -> mouse_state == RightClick) map_output_val = RIGHT_CLICK_MAP_PLACEMENT_VALUE;
		else map_output_val = eds -> in_texture_editing_mode ? eds -> editor_texture_id : eds -> editor_height;

		*map_point(eds, !eds -> in_texture_editing_mode,
			eds -> tile_pos[0], eds -> tile_pos[1]) = map_output_val;
	}
}

static void render_eds_map(const EditorState* const eds) {
	const byte map_width = eds -> map_size[0], map_height = eds -> map_size[1];
	const int mouse_x = eds -> mouse_pos[0], mouse_y = eds -> mouse_pos[1];

	const float
		scr_blocks_across = (float) EDITOR_WIDTH / map_width,
		scr_blocks_down = (float) EDITOR_MAP_SECTION_HEIGHT / map_height;

	const int
		ceil_src_blocks_across = ceilf(scr_blocks_across),
		ceil_src_blocks_down = ceilf(scr_blocks_down);

	byte highest_point = 0;
	for (uint16_t i = 0; i < map_width * map_height; i++) {
		const byte height = eds -> heightmap[i];
		if (height > highest_point) highest_point = height;
	}

	const byte height_shade_range = BIGGEST_HEIGHT_SHADE - SMALLEST_HEIGHT_SHADE;

	for (byte map_y = 0; map_y < map_height; map_y++) {
		for (byte map_x = 0; map_x < map_width; map_x++) {
			SDL_Texture* const texture = eds -> textures[*map_point(eds, 0, map_x, map_y)];

			const SDL_Rect block_pos = {
				map_x * scr_blocks_across, map_y * scr_blocks_down,
				ceil_src_blocks_across, ceil_src_blocks_down
			};

			const float height_ratio = (float) *map_point(eds, 1, map_x, map_y) / highest_point;
			byte height_shade = height_ratio * height_shade_range + SMALLEST_HEIGHT_SHADE;

			// Selected block shaded more. Condition will equal 1 or 0, and right shift by 1 divides by 2.
			height_shade >>= (mouse_x >= block_pos.x && mouse_x < block_pos.x + block_pos.w
				&& mouse_y >= block_pos.y && mouse_y < block_pos.y + block_pos.h);

			SDL_SetTextureColorMod(texture, height_shade, height_shade, height_shade);
			SDL_RenderCopy(eds -> renderer, texture, NULL, &block_pos);
		}
	}
}

static void editor_loop(EditorState* const eds) {
	const int16_t max_delay = 1000 / EDITOR_FPS;

	SDL_Renderer* const renderer = eds -> renderer;

	InfoBar info_bar;
	init_info_bar(&info_bar, renderer);

	while (1) {
		const Uint32 before = SDL_GetTicks();
		SDL_Event event;

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_QUIT:
					deinit_info_bar(&info_bar);
					return;
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == KEY_CLICK_TILE) eds -> mouse_state = LeftClick;
					else if (event.button.button == KEY_ERASE_TILE) eds -> mouse_state = RightClick;
					break;
				case SDL_MOUSEBUTTONUP:
					eds -> mouse_state = NoClick;
					break;
				case SDL_KEYDOWN:
					update_editing_placement_values(eds, &event);
					break;
			}
		}

		SDL_GetMouseState(eds -> mouse_pos, eds -> mouse_pos + 1);
		eds -> tile_pos[0] = (float) eds -> mouse_pos[0] / EDITOR_WIDTH * eds -> map_size[0];
		eds -> tile_pos[1] = (float) eds -> mouse_pos[1] / EDITOR_MAP_SECTION_HEIGHT * eds -> map_size[1];
		if (eds -> tile_pos[1] >= eds -> map_size[1]) eds -> tile_pos[1] = eds -> map_size[1] - 1;

		edit_eds_map(eds);
		SDL_RenderClear(renderer);
		render_eds_map(eds);
		render_info_bar(&info_bar, eds);
		SDL_RenderPresent(renderer);

		const Uint32 ms_elapsed = SDL_GetTicks() - before;
		const int16_t wait_for_exact_fps = max_delay - ms_elapsed;
		if (wait_for_exact_fps > 0) SDL_Delay(wait_for_exact_fps);
	}
}

void init_editor_state(EditorState* const eds, SDL_Renderer* const renderer) {
	init_editor_state_from_json_file(eds, "src/data/simple.json");

	enum {
		num_textures = 11, map_width = palace_width, map_height = palace_height
		// num_textures = 3, map_width = pyramid_width, map_height = pyramid_height
		};

	const char* const texture_paths[num_textures] = {
		/* "../../../../assets/walls/pyramid_bricks_4.bmp",
		"../../../../assets/walls/greece.bmp", "../../../../assets/walls/saqqara.bmp" */

		"../../../../assets/walls/sand.bmp", "../../../../assets/walls/pyramid_bricks_4.bmp",
		"../../../../assets/walls/marble.bmp", "../../../../assets/walls/hieroglyph.bmp",
		"../../../../assets/walls/window.bmp", "../../../../assets/walls/saqqara.bmp",
		"../../../../assets/walls/sandstone.bmp", "../../../../assets/walls/cobblestone_3.bmp",
		"../../../../assets/walls/horses.bmp", "../../../../assets/walls/mesa.bmp",
		"../../../../assets/walls/arthouse_bricks.bmp"
	};

	// static byte _texture_id_map[map_height][map_width];
	byte* const heightmap = (byte*) palace_heightmap, *const texture_id_map = (byte*) palace_texture_id_map;
	// byte* const heightmap = (byte*) pyramid_heightmap, *const texture_id_map = (byte*) pyramid_texture_id_map;

	//////////

	const size_t map_bytes = map_width * map_height;

	eds -> num_textures = num_textures;
	eds -> map_size[0] = map_width;
	eds -> map_size[1] = map_height;
	eds -> tile_pos[0] = 0;
	eds -> tile_pos[1] = 0;
	eds -> in_texture_editing_mode = 1;
	eds -> editor_texture_id = INIT_EDITOR_TEXTURE_ID;
	eds -> editor_height = INIT_EDITOR_HEIGHT;
	eds -> mouse_state = NoClick;

	eds -> heightmap = malloc(map_bytes);
	memcpy(eds -> heightmap, heightmap, map_bytes);
	eds -> texture_id_map = malloc(map_bytes);
	memcpy(eds -> texture_id_map, texture_id_map, map_bytes);

	eds -> map_name = "palace";

	eds -> renderer = renderer;
	eds -> textures = malloc(num_textures * sizeof(SDL_Texture*));

	for (byte i = 0; i < num_textures; i++)
		eds -> textures[i] = init_texture(texture_paths[i], renderer, 1);
}

void deinit_editor_state(EditorState* const eds) {
	free(eds -> heightmap);
	free(eds -> texture_id_map);

	for (byte i = 0; i < eds -> num_textures; i++)
		SDL_DestroyTexture(eds -> textures[i]);
	free(eds -> textures);
}

int main(void) {
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
		FAIL(LaunchSDL, "SDL error: \"%s\".", SDL_GetError());
	
	SDL_Window* window;
	SDL_Renderer* renderer;

	if (SDL_CreateWindowAndRenderer(EDITOR_WIDTH, EDITOR_HEIGHT, 0, &window, &renderer) == -1)
		FAIL(LaunchSDL, "Window or renderer creation failure: \"%s\".", SDL_GetError());

	SDL_SetWindowTitle(window, APP_NAME);

	EditorState eds;
	init_editor_state(&eds, renderer);
	editor_loop(&eds);
	deinit_editor_state(&eds);

	SDL_DestroyRenderer(eds.renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}
