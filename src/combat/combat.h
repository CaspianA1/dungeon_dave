enum {max_route_length = 12, route_queue_init_alloc = 5};

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

inlinable Navigator init_navigator(const vec, vec* const, const double);

//////////

typedef enum {
	Idle, Chasing, Attacking, Dead
} EnemyState;

typedef struct {
	const double dist_wake_from_idle, power, init_hp;
	const byte animation_seg_lengths[4];

	const char* const animation_spritesheet_path;
	const DataAnimation animation_data;
} Enemy;

typedef struct {
	EnemyState state;
	const double dist_wake_from_idle, power;
	double hp, time_at_attack;

	byte recently_attacked, animation_seg_lengths[4];
	AnimatedBillboard animated_billboard; // from one large spritesheet

	Sound sounds[5]; // a sound for each state + Attacked
	Navigator nav;
} EnemyInstance;

//////////

typedef struct {
	// byte in_use, short_range, paces_sideways, recently_used; // in the last tick
	byte status; // in_use, short_range, paces_sideways, recently_used
	const double power, dist_for_hit;
	Sound sound;
	DataAnimation animation_data;
} Weapon;

//////////
