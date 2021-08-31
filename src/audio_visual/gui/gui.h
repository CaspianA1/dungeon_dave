typedef struct {
	TTF_Font* font;
	Sprite sprite;
	byte r, g, b, has_background;
	SDL_Rect pos;
} OldMessage;

typedef struct {
	byte enabled, enabled_previously;
	const int key;
} Toggle;

#define toggledef(key)\
	static Toggle toggle = {0, 0, key};\
	if (!update_toggle(&toggle)) return;

const char* const STD_GUI_FONT_PATH = "assets/dnd.ttf";
