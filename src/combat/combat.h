typedef struct {
	VectorI* data;
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

typedef struct {
	Path path_to_player; // whole-number path
	VectorF* const pos;
	double* const dist_to_player;
	int path_ind;
	const double v;
} Navigator;

inlinable Navigator init_navigator(const VectorF, VectorF* const, double* const, const double);

/////

typedef struct {
	const double
		min_idle_sound, max_idle_sound,
		begin_chasing, begin_attacking;
} EnemyDistThresholds;

typedef enum {
	Idle, Chasing, Attacking, Retreating, Dead
} EnemyState;

typedef struct {
	EnemyState state;
	const EnemyDistThresholds dist_thresholds;
	const double hp_to_retreat;
	double hp;

	// the pos is stored in the Animation, and the Navigator stores a pointer to that

	/* A sound for each state. Each sound plays
	when the state begins. Retreating sound = chasing sound. */
	const byte animation_seg_lengths[5];
	Animation animations; // from one large spritesheet

	Sound* const restrict sounds; // 5 sounds (ptrs b/c struct doubles in size otherwise)
	Navigator nav;
} Enemy;

/////

typedef struct {
	byte in_use;
	const double screen_y_shift_percent_down;
	Sound sound;
	Animation animation;
} Weapon;

inlinable Weapon init_weapon(const char*, const char*, const double, const int, const int, const int, const int);

/////
