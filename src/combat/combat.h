typedef struct {
	ivec* data;
	int length, max_alloc;
} Path;

typedef struct {
	Path* data;
	int length, max_alloc;
} PathQueue;

typedef struct {
	byte succeeded;
	Path path;
} ResultBFS;

/////

typedef enum {
	Navigating, ReachedDest, CouldNotNavigate
} NavigatorState;

/* this path accomodates for the previous pos of the Navigator
and aligns the navigator to the middle of the tile it's on */
typedef struct {
	vec* data;
	int length;
} CorrectedPath;

typedef struct {
	CorrectedPath path;
	vec* const pos;
	int path_ind;
	const double v;
} Navigator;

inlinable Navigator init_navigator(const vec, vec* const, const double);

/////

typedef enum {
	Idle, Chasing, Attacking, Dead
} EnemyState;

typedef struct {
	EnemyState state;
	const double dist_wake_from_idle, dist_return_to_idle;
	double hp;

	byte recently_attacked;
	const byte animation_seg_lengths[4];
	Animation animations; // from one large spritesheet

	Sound* const sounds; // a sound for each state + Attacked (ptrs b/c the struct doubles in size otherwise)
	Navigator nav;
} Enemy;

/////

typedef struct {
	byte in_use;
	const double power, dist_for_hit;
	Sound sound;
	Animation animation;
} Weapon;

inlinable Weapon init_weapon(const char* const, const char* const,
	const double, const double, const int, const int, const int, const int);

/////
