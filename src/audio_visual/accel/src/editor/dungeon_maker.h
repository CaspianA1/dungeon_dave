#ifndef DUNGEON_MAKER_H
#define DUNGEON_MAKER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define EDITOR_W 1200
#define EDITOR_H 700
#define EDITOR_MAP_SECTION_H 620

#define EDITOR_FPS 60
#define SELECTED_TILE_COLOR_MOD 127, 127, 127

// #define FONT_PATH "monospaci.py/Fontgoodexport-Mono.ttf"
#define FONT_PATH "../../../../assets/dnd.ttf"
// #define FONT_PATH "mono_dnd.otf"

#define KEY_TOGGLE_TEXTURE_EDIT_MODE SDL_SCANCODE_ESCAPE
#define KEY_CLICK_TILE SDL_BUTTON_LEFT

#define DEBUG(var, format) printf(#var " = %" #format "\n", var)

#define FAIL(failure_type, format, ...) do {\
	fprintf(stderr, "Failed with error type %s. Message: "\
		format "\n", #failure_type, __VA_ARGS__);\
	exit(failure_type + 1);\
} while (0)

//////////

typedef uint_fast8_t byte;

typedef enum {
	LaunchSDL,
	OpenFile,
	CreateTexture,
	CreateTextSurface
} FailureType;

typedef enum {
	LeftClick,
	RightClick,
	NoClick
} MouseState;

typedef struct {
	byte
		num_textures, map_size[2], tile_pos[2],
		in_texture_editing_mode,
		// These indicate the height and texture id placed while editing
		editing_texture_id, editing_height,
		*heightmap, *texture_id_map;

	MouseState mouse_state;
	int mouse_pos[2];

	SDL_Texture** textures;
	SDL_Renderer* renderer;
} EditorState;

#endif
