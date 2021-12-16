#include <SDL2/SDL.h>

#define EDITOR_W 1000
#define EDITOR_H 700
#define EDITOR_MAP_SECTION_H 650
#define EDITOR_FPS 60
#define TEX_SHADED_BLOCK_COLOR_MOD 127, 127, 127

#define KEY_TOGGLE_TEXTURE_EDIT_MODE SDL_SCANCODE_T
#define KEY_CLICK_BLOCKS SDL_BUTTON_LEFT

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
	CreateTexture
} FailureType;

typedef enum {
	LeftClick,
	RightClick,
	NoClick
} MouseState;

typedef struct {
	byte
		num_textures, map_size[2],
		in_texture_editing_mode,
		*heightmap, *texture_id_map;

	int mouse_pos[2];

	SDL_Texture** textures;
} EditorState;
