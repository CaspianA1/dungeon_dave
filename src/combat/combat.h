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
	const double wake_from_idle, max_idle_sound;
} EnemyDistThresholds;

typedef enum {
	Idle, Chasing, Attacking, Dead
} EnemyState;

typedef struct {
	EnemyState state;
	const EnemyDistThresholds dist_thresholds;
	const double hp_to_retreat;
	double hp;

	const byte animation_seg_lengths[4];
	Animation animations; // from one large spritesheet

	// A sound for each state. Each sound plays when the state begins.
	Sound* const sounds; // 5 sounds (ptrs b/c struct doubles in size otherwise)
	Navigator nav;
} Enemy;

/////

typedef struct {
	byte in_use;
	const double screen_y_shift_percent_down;
	Sound sound;
	Animation animation;
} Weapon;

inlinable Weapon init_weapon(const char* const, const char* const, const double, const int, const int, const int, const int);

/////
