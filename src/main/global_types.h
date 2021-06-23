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
	double v, max_v_reached, a, limit_v, strafe_v,
		time_of_move, time_of_stop, v_incr_multiplier;
} KinematicBody;

typedef struct {
	byte jumping;
	const double up_v0;
	double v0, height, start_height, highest_height, time_at_jump;
	const Sound sound_at_jump, sound_at_land;
} Jump;

typedef struct {
	VectorF pos;
	VectorI mouse_pos;
	double angle;
	int z_pitch;
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
	double* z_buffer;
} Screen;

/////

typedef struct {
	int screen_width, screen_height, half_screen_width, half_screen_height,
		max_fps, max_delay, ray_column_width;

	double fov, fov_step, max_fov, proj_dist, minimap_scale, pace_max_divisor, stop_dist_from_wall;
} Settings;

/////

typedef struct {
	const int map_width, map_height;
	const VectorF init_pos;
	const double init_height;

	byte
		wall_count, billboard_count, animation_count, enemy_count,
		generic_billboard_count, max_point_height,
 		*wall_data, *ceiling_data, *floor_data;

	Skybox skybox;
	Sound background_sound;

	byte (*get_point_height) (const byte, const VectorF);
	double (*shader) (const VectorF);
	// for cache optimization; `shader` and `walls` are typically used together

	Sprite* walls;
	Billboard* billboards;
	Animation* animations;
	Enemy* enemies;
	GenericBillboard* generic_billboards;
} Level;

/////


Screen screen;
Level current_level;
Settings settings;

SDL_Event event;
const Uint8* keys;
