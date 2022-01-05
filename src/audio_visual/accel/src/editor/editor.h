#ifndef EDITOR_H
#define EDITOR_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define EDITOR_WIDTH 1000
#define EDITOR_HEIGHT 750
#define EDITOR_MAP_SECTION_HEIGHT 650
#define EDITOR_FPS 60

#define SMALLEST_HEIGHT_SHADE 100
#define BIGGEST_HEIGHT_SHADE 255

#define FONT_PATH "../../../../assets/dnd.ttf"
// #define FONT_PATH "monodnd.ttf"

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
	CreateTextSurface,
	ParseLevelFile
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
	
	const char* map_name;

	MouseState mouse_state;
	int mouse_pos[2];

	SDL_Texture** textures;
	SDL_Renderer* renderer;
} EditorState;

#endif
