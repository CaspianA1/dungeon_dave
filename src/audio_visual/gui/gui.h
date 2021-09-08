typedef struct {
	byte r, g, b;
} Color3;

typedef struct {
	byte enabled, enabled_previously;
	const int key;
} Toggle;

#define toggledef(key)\
	static Toggle toggle = {0, 0, key};\
	if (!update_toggle(&toggle)) return;

const char* const gui_font_path = "assets/dnd.ttf";
const byte font_size_divisor = 10;
