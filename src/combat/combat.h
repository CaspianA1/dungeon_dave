enum {max_route_length = 10, route_queue_init_alloc = 5};

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
	EnemyState state;
	const double dist_wake_from_idle;
	double hp, power, time_at_attack;

	byte recently_attacked;
	const byte animation_seg_lengths[4];
	Animation animations; // from one large spritesheet

	Sound* const sounds; // a sound for each state + Attacked (ptrs b/c the struct doubles in size otherwise)
	Navigator nav;
} Enemy;

//////////

typedef struct {
	byte in_use, short_range, paces_sideways;
	const double power, dist_for_hit;
	Sound sound;
	Animation animation;
} Weapon;

//////////
