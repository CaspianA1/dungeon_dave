#ifndef EDITOR_H
#define EDITOR_H

#include <SDL2/SDL.h>
#include "../headers/buffer_defs.h"

#define EDITOR_WIDTH 1440
#define EDITOR_HEIGHT 720
#define EDITOR_MAP_SECTION_HEIGHT 650
#define EDITOR_FPS 60

#define MAX_BYTE_VALUE 255

#define SMALLEST_HEIGHT_SHADE 100
#define BIGGEST_HEIGHT_SHADE 255

#define RIGHT_CLICK_MAP_PLACEMENT_VALUE 0
#define INIT_EDITOR_TEXTURE_ID 1
#define INIT_EDITOR_HEIGHT 1

#define KEY_TOGGLE_TEXTURE_EDIT_MODE SDL_SCANCODE_ESCAPE
#define KEY_CLICK_TILE SDL_BUTTON_LEFT
#define KEY_ERASE_TILE SDL_BUTTON_RIGHT

#define DEBUG(var, format) printf(#var " = %" #format "\n", var)

#define FAIL(failure_type, format, ...) do {\
	fprintf(stderr, "Failed with error type %s. Message: "\
		format "\n", #failure_type, __VA_ARGS__);\
	exit(failure_type + 1);\
} while (0)

//////////

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

/* This should be freed from EditorState:
	- heightmap, texture_id_map, and map_name, via free
	- textures, via SDL_DestroyTexture
	- renderer, via SDL_DestroyRenderer */
typedef struct {
	byte
		num_textures, map_size[2], tile_pos[2],
		editor_texture_id, editor_height, // These indicate the height and texture id placed while editing
		*heightmap, *texture_id_map;

	bool in_texture_editing_mode;

	const char* map_name;

	MouseState mouse_state;
	int mouse_pos[2];

	SDL_Renderer* renderer;
	SDL_Texture** textures;
} EditorState;

//////////

// Excluded: update_editing_placement_values, edit_eds_map, render_eds_map, editor_loop

byte* map_point(const EditorState* const eds, const bool is_heightmap, const byte x, const byte y);
SDL_Texture* init_texture(const char* const path, SDL_Renderer* const renderer, const bool linear_filtering);
int16_t three_chars_to_int(const char chars[3], const byte num_chars_to_convert);

void init_editor_state(EditorState* const eds, SDL_Renderer* const renderer); // Expected to be used publicly later
void deinit_editor_state(EditorState* const eds);

#endif
