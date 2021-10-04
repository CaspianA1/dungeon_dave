enum {max_route_length = 16, route_queue_init_alloc = 4};

typedef struct {
	byte creation_error;
	int length;
	ivec data[max_route_length];
} Route;

typedef struct {
	Route* data;
	int length, max_alloc;
} RouteQueue;

typedef enum {
	FailedBFS, SucceededBFS, PathTooLongBFS,
	Navigating, ReachedDest
} NavigationState;

typedef struct {
	NavigationState state;
	Route route;
} ResultBFS;

//////////

/* this path accomodates for the previous pos of the Navigator
and aligns the navigator to the middle of the tile it's on */
typedef struct {
	int length;
	vec data[max_route_length];
} CorrectedRoute;

typedef struct {
	CorrectedRoute route;
	vec* const pos;
	int route_ind;
	const double v;
} Navigator;

inlinable Navigator init_navigator(const vec, vec* const, const double, const byte);

//////////

typedef enum {
	Idle, Chasing, Attacking, Dead
} EnemyState;

typedef struct {
	const double power, init_hp, nav_speed;

	const struct {const byte sight, sound;} dist_awaken;
	const byte is_short_range, animation_seg_lengths[4];

	const DataAnimationImmut animation_data;
	const Sound sounds[5]; // a sound for each state + Attacked
} Enemy;

typedef struct {
	const Enemy* const enemy;
	DataAnimationMut mut_animation_data;

	EnemyState state;
	byte status; // recently attacked, long range, weapon y pitch is in range of enemy
	double hp, time_at_attack;
	DataBillboard billboard_data;
	Navigator nav;
} EnemyInstance;

//////////

typedef struct {
	byte status; // in_use, short_range, paces_sideways, recently_used (in the last tick)
	const double power;
	const Sound sound;
	DataAnimation animation_data;
} Weapon;

//////////
