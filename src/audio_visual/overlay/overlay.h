typedef struct {
	SDL_Surface* restrict surface;
	SDL_Texture* restrict texture;
} Sprite;

/////

typedef struct {
	Sprite sprite;
	VectorF pos;
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

typedef struct {
	Billboard billboard;
	byte is_animated, is_enemy;
	int animation_index;
} GenericBillboard;

/////

typedef struct {
	byte enabled;
	Sprite sprite;
	int max_width, max_height;
} Skybox;
