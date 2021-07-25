typedef struct {
	TTF_Font* font;
	Sprite sprite;
	byte r, g, b, has_background;
	SDL_Rect pos;
} Message;

/////

typedef struct {
	byte enabled, enabled_previously;
	const int key;
} Toggle;
