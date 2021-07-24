typedef struct {
	SDL_Texture* texture;
	ivec size;
} Sprite;

Sprite init_sprite(const char* const, const byte);
#define deinit_sprite(sprite) SDL_DestroyTexture(sprite.texture);
#define deinit_psprite(p) SDL_DestroyTexture(p.texture);

typedef struct {
	void* pixels;
	int pitch, size; // for pixelwise access, equal dimensions are needed
	SDL_Texture* texture;
} PSprite; // pixelwise access

/////

typedef struct {
	Sprite sprite;
	vec pos;
	double beta, dist, height; // `height` here is a point height
} Billboard;

/////

typedef struct {
	Billboard billboard;

	const int frames_per_row, frames_per_col, frame_w, frame_h, frame_count;

	int frame_ind;
	const double secs_per_frame;
	double last_frame_time;
} Animation;

Animation init_animation(const char* const, const int, const int, const int, const int, const byte);

typedef struct {
	Billboard billboard;
	byte is_animated, is_enemy;
	int animation_index;
} GenericBillboard;

/////

typedef struct {
	byte enabled;
	Sprite sprite;
} Skybox;
