typedef struct {
	SDL_Rect (*pos) (void);
	void (*on_click) (void);
	const SDL_Texture* const rendered_text;
} Textbox;

typedef struct {
	const TTF_Font* const font;
	Textbox* const textboxes;
	const Color3 bg_color, fg_color; // foreground color is for textboxes
} Menu;

// variadic params: pos fn, on_click fn
Menu init_menu(const Color3 bg_color, const Color3 fg_color,
	const ivec textbox_size, const unsigned textbox_count, ...);

Menu deinit_menu(const Menu* const menu);

void render_menu(const Menu* const menu);
