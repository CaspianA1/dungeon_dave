typedef enum {
	Exit, ProceedAsNormal, BeginAnimatingWeapon, NextScreen, ToOptionsMenu
} InputStatus;

//////////

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
	byte status; // moving forward or backward, was forward, was backward
	double v, max_v_reached, a, limit_v, strafe_v, time_of_move, time_of_stop, v_incr_multiplier;
} KinematicBody;

typedef struct {
	byte jumping, made_noise;
	const double up_v0;
	double v0, height, start_height, highest_height, time_at_jump;
	const Sound sound_at_jump, sound_at_land;
} Jump;

typedef struct {
	vec pos, dir;
	double angle, hp;
	byte is_dead;
	int y_pitch;
	Sound sound_when_attacked, sound_when_dying;
	Jump jump;
	Domain tilt; // tilt is in degrees, while other values are in radians b/c SDL_RenderCopyEx uses degrees
	Pace pace;
	KinematicBody body;
} Player;

//////////

typedef struct {
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_PixelFormat* pixel_format;
	SDL_Texture *pixel_buffer, *shape_buffer;
	void* pixels;
	int pixel_pitch; // `pixels` and `pixel_pitch` pertain to the pixel buffer
} Screen;

typedef struct {
	float one_over_cos_beta;
	vec dir;
} FloorcastBufferVal;

//////////

typedef struct {
	int screen_width, screen_height, half_screen_width, half_screen_height,
		avg_dimensions, max_fps, max_delay, ray_column_width;

	double init_fov, fov, fov_step, max_fov, proj_dist, minimap_scale;
} Settings;

//////////

typedef struct {
	ivec chunk_dimensions;
	int alloc_bytes;
	byte* data;
} StateMap;

//////////

typedef struct {
	DataBillboard from_billboard;
	vec to;
} Teleporter;

//////////

typedef struct {
	byte* data;
	ivec size;
} Lightmap;

//////////

typedef struct {
	ivec map_size;
	vec init_pos;
	double init_height;

	byte
		*wall_data, *heightmap, /* *ceiling_data, *floor_data */
		wall_count, billboard_count, animated_billboard_count, enemy_instance_count,
		teleporter_count, thing_count, max_point_height, out_of_bounds_point;

 	StateMap bfs_visited;

	Skybox skybox;
	Sound background_sound;

	double (*shader) (const vec);

	#ifdef SHADING_ENABLED
	Lightmap lightmap;
	#endif

	Sprite* walls;
	Billboard* billboards;
	AnimatedBillboard* animated_billboards;
	EnemyInstance* enemy_instances;
	Teleporter* teleporters;
	Thing* thing_container;
} Level;

//////////

Screen screen;
Settings settings;
Level current_level;

FloorcastBufferVal* floorcast_val_buffer;
float* depth_buffer;
StateMap occluded_by_walls;

SDL_Event event;
const Uint8* keys;
