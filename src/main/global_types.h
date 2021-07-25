typedef enum {
	Exit, ProceedAsNormal, BeginAnimatingWeapon
} InputStatus;

/////

typedef struct {
	double val;
	const double step;
	double max;
} Domain;

typedef struct {
	Domain domain;
	const double offset_scaler;
	double screen_offset;
} Pace;

typedef struct {
	byte moving_forward_or_backward, was_forward, was_backward;
	double v, max_v_reached, a, limit_v, strafe_v, time_of_move, time_of_stop, v_incr_multiplier;
} KinematicBody;

typedef struct {
	byte jumping;
	const double up_v0;
	double v0, height, start_height, highest_height, time_at_jump;
	const Sound sound_at_jump, sound_at_land;
} Jump;

typedef struct {
	vec pos, dir;
	ivec mouse_pos;
	double angle, hp;
	const double init_hp;
	byte is_dead;
	int y_pitch;
	Jump jump;
	Domain tilt;
	Pace pace;
	KinematicBody body;
} Player;

/////

typedef struct {
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_PixelFormat* pixel_format;
	SDL_Texture *pixel_buffer, *shape_buffer;
	void* pixels;
	int pixel_pitch; // `pixels` and `pixel_pitch` pertain to the pixel buffer
} Screen;

typedef struct {
	float depth, cos_beta;
	vec dir;
} BufferVal;

/////

typedef struct {
	int screen_width, screen_height, half_screen_width, half_screen_height,
		max_fps, max_delay, ray_column_width;

	double fov, fov_step, max_fov, proj_dist, minimap_scale, stop_dist;
} Settings;

/////

typedef struct {
	ivec chunk_dimensions;
	int alloc_bytes;
	byte* data;
} StateMap;

/////

typedef struct {
	const ivec map_size;
	const vec init_pos;
	const double init_height;

	byte
		wall_count, billboard_count, animation_count, enemy_count,
		generic_billboard_count, max_point_height, out_of_bounds_point,
 		*wall_data, *ceiling_data, *floor_data;

 	StateMap bfs_visited;

	Skybox skybox;
	Sound background_sound;

	byte (*get_point_height) (const byte, const vec);
	double (*shader) (const vec);
	// for cache optimization; `shader` and `walls` are typically used together

	Sprite* walls;
	Billboard* billboards;
	Animation* animations;
	Enemy* enemies;
	GenericBillboard* generic_billboards; // just a temp container that other billboards are copied into
} Level;

/////

Screen screen;
BufferVal* val_buffer;
StateMap occluded_by_walls;
Settings settings;
Level current_level;

SDL_Event event;
const Uint8* keys;
