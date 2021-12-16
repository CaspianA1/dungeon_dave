#include "dungeon_maker.h"
#include "../data/maps.c"

/*
- 2 options: start new, and edit current. Begin with start new.
- A bottom bar that gives the current texture and current height
- Press a number key to change the current tex

Plan:
- Press 't' to toggle texture editing mode
- Input a series of numbers, and then hit return (stopping if more than 3 numbers), to select a height or texture id number
- If number over 255, limit to that

- Variables editing_height and editing_texture_id
- Drag and click to draw to a map

- Press 'e' to enter erase mode
- Dragging and clicking will erase tiles from the current map, setting them to 0

- Later on, line and rectangle functions (or maybe just a line function)

- Selected block highlighted - done
*/

byte* map_point(const EditorState* const eds, const byte is_heightmap, const byte x, const byte y) {
	byte* const map = is_heightmap ? eds -> heightmap : eds -> texture_id_map;
	return map + (y * eds -> map_size[0] + x);
}

void edit_eds_map(const EditorState* const eds) {
	// puts("Edit shit. Press 't' to toggle texture editing mode.");
	(void) eds;

	static byte first_call = 1;
	static const Uint8* keys;

	if (first_call) {
		keys = SDL_GetKeyboardState(NULL);
		first_call = 0;
	}

	// DEBUG(keys[SDL_SCANCODE_C], d);
}

void render_eds_map(const EditorState* const eds, SDL_Renderer* const renderer) {
	const byte map_width = eds -> map_size[0], map_height = eds -> map_size[1];

	const float
		scr_blocks_across = (float) EDITOR_W / map_width,
		scr_blocks_down = (float) EDITOR_MAP_SECTION_H / map_height;

	int mouse_x, mouse_y;
	SDL_GetMouseState(&mouse_x, &mouse_y);

	for (byte map_y = 0; map_y < map_height; map_y++) {
		for (byte map_x = 0; map_x < map_width; map_x++) {
			const byte texture_id = *map_point(eds, 0, map_x, map_y);
			SDL_Texture* const texture = eds -> textures[texture_id];

			const SDL_FRect block_pos = {
				map_x * scr_blocks_across, map_y * scr_blocks_down,
				scr_blocks_across, scr_blocks_down
			};

			const byte highlight_texture =
				mouse_x >= block_pos.x && mouse_x < block_pos.x + block_pos.w
				&& mouse_y >= block_pos.y && mouse_y < block_pos.y + block_pos.h;
			
			if (highlight_texture) SDL_SetTextureColorMod(texture, TEX_SHADED_BLOCK_COLOR_MOD);
			SDL_RenderCopyF(renderer, texture, NULL, &block_pos);
			if (highlight_texture) SDL_SetTextureColorMod(texture, 255, 255, 255);
		}
	}
}

void editor_loop(EditorState* const eds, SDL_Renderer* const renderer) {
	const int16_t max_delay = 1000 / EDITOR_FPS;
	byte editing = 1;

	while (editing) {
		const Uint32 before = SDL_GetTicks();
		SDL_Event event;

		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) editing = 0;
		}

		edit_eds_map(eds);
		render_eds_map(eds, renderer);
		SDL_RenderPresent(renderer);

		const Uint32 ms_elapsed = SDL_GetTicks() - before;
		const int16_t wait_for_exact_fps = max_delay - ms_elapsed;
		if (wait_for_exact_fps > 0) SDL_Delay(wait_for_exact_fps);
	}
}

void init_editor_state(EditorState* const eds, SDL_Renderer* const renderer) {
	// Hardcoded for now

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

	eds -> num_textures = num_textures;
	eds -> map_size[0] = map_width;
	eds -> map_size[1] = map_height;
	eds -> in_texture_editing_mode = 0;
	eds -> heightmap = heightmap;
	eds -> texture_id_map = (byte*) texture_id_map;
	eds -> textures = malloc(num_textures * sizeof(SDL_Texture*));

	for (byte i = 0; i < num_textures; i++) {
		const char* const path = texture_paths[i];
		SDL_Surface* const surface = SDL_LoadBMP(path);
		if (surface == NULL) FAIL(OpenFile, "Surface with path '%s' not found", path);

		SDL_Texture* const texture = SDL_CreateTextureFromSurface(renderer, surface);
		if (texture == NULL) FAIL(CreateTexture, "Could not create a texture from surface with path '%s'", path);

		eds -> textures[i] = texture;

		SDL_FreeSurface(surface);
	}
}

void deinit_editor_state(EditorState* const eds) {
	for (byte i = 0; i < eds -> num_textures; i++)
		SDL_DestroyTexture(eds -> textures[i]);
}

int main(void) {
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
		FAIL(LaunchSDL, "SDL error: \"%s\".", SDL_GetError());
	
	SDL_Window* window;
	SDL_Renderer* renderer;

	if (SDL_CreateWindowAndRenderer(EDITOR_W, EDITOR_H, 0, &window, &renderer) == -1)
		FAIL(LaunchSDL, "Window or renderer creation failure: \"%s\".", SDL_GetError());

	SDL_SetHintWithPriority(SDL_HINT_RENDER_SCALE_QUALITY, "1", SDL_HINT_OVERRIDE);	
	SDL_SetWindowTitle(window, "Dungeon Maker");
	
	EditorState eds;
	init_editor_state(&eds, renderer);
	editor_loop(&eds, renderer);
	deinit_editor_state(&eds);

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}
