#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <pthread.h>

#define SHADING_ENABLED
#define SOUND_ENABLED
#define FULL_QUALITY
// #define TRACK_MEMORY
// #define NOCLIP_MODE

#define INIT_W 800
#define INIT_H 600
#define INIT_MAX_FPS 60
#define INIT_RAY_COLUMN_W 1

// #define INIT_FOV 66.849622365373434718
#define INIT_FOV 90.0
#define INIT_FOV_STEP 0.5
#define INIT_MAX_FOV 100.0

#define INIT_MINIMAP_SCALE 3.0
#define INIT_PACE_MAX_DIVISOR 1.2
#define INIT_STOP_DIST_FROM_WALL 0.1

#define AUDIO_CHUNK_SIZE 1024

#define KEY_DISABLE_MINIMAP SDL_SCANCODE_1
#define KEY_ENABLE_MINIMAP SDL_SCANCODE_2
#define KEY_SPEEDUP_1 SDL_SCANCODE_LSHIFT
#define KEY_SPEEDUP_2 SDL_SCANCODE_RSHIFT
#define KEY_JUMP SDL_SCANCODE_SPACE
#define KEY_FLY_UP SDL_SCANCODE_O
#define KEY_FLY_DOWN SDL_SCANCODE_L
#define KEY_USE_WEAPON SDL_BUTTON_LEFT
#define KEY_FORWARD SDL_SCANCODE_W
#define KEY_BACKWARD SDL_SCANCODE_S
#define KEY_LSTRAFE SDL_SCANCODE_A
#define KEY_RSTRAFE SDL_SCANCODE_D

#define PIXEL_FORMAT SDL_PIXELFORMAT_ARGB8888
#define PIXEL_FORMAT_BPP 4

#define inlinable static inline

#define LOOP(max) for (int i = 0; i < max; i++)
#define FAIL(...) {fprintf(stderr, __VA_ARGS__); exit(1);}
#define DEBUG(var, format) printf(#var " = %" #format "\n", var)
#define DEBUG_VECF(vec) printf(#vec " = {%lf, %lf}\n", vec[0], vec[1])
#define DEBUG_VECI(vec) printf(#vec " = {%d, %d}\n", vec.x, vec.y)

/////

const double
	two_pi = M_PI * 2.0,
	half_pi = M_PI / 2.0,
	three_pi_over_two = 3.0 * M_PI / 2.0,
	five_pi_over_two = 5.0 * M_PI / 2.0,
	g = -9.8,
	std_double_epsilon = 0.01,
	small_double_epsilon = 0.000001;

typedef uint_fast8_t byte;
typedef byte* restrict const* restrict map_data;
typedef byte** mut_map_data;
typedef __m128d VectorF; // two doubles
typedef __m256d VectorF2; // four doubles

typedef struct {
	int x, y;
} VectorI;

// https://www.spriters-resource.com/pc_computer/doomdoomii/
// https://lodev.org/cgtutor/raycasting4.html
// https://medium.com/@Powersaurus/pico-8-raycaster-doors-cd8de9d943b
