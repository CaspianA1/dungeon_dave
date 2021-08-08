typedef struct {
	SDL_Texture* texture;
	ivec size;
	byte max_mipmap_depth;
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
	vec pos;
	double beta, dist, height; // `height` here is a point height
} DataBillboard;

typedef struct {
	Sprite sprite;
	DataBillboard billboard_data;
} Billboard;

typedef struct {
	const Sprite sprite;
	// const int frames_per_row, frames_per_col, frame_w, frame_h, frame_count;
	const ivec frames_per_axis, frame_size;
	const int frame_count;
	const double secs_per_frame;
} DataAnimationImmut;

typedef struct {
	double last_frame_time;
	int frame_ind;
} DataAnimationMut;

typedef struct {
	const DataAnimationImmut immut;
	DataAnimationMut mut;
} DataAnimation;

typedef struct {
	DataAnimation animation_data;
	DataBillboard billboard_data;
} AnimatedBillboard;

typedef struct {
	const DataBillboard* const billboard_data;
	const Sprite sprite;
	const SDL_Rect src_crop;
} Thing;

DataAnimation init_animation_data(const char* const, const int, const int, const int, const int, const byte);

/////

typedef struct {
	byte enabled;
	Sprite sprite;
} Skybox;
