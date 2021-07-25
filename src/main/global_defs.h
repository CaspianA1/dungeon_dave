#ifdef __clang__

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>

#else

#include "/usr/local/include/SDL2/SDL.h"
#include "/usr/local/include/SDL2/SDL_ttf.h"
#include "/usr/local/include/SDL2/SDL_mixer.h"

#endif

/*
#define PLANAR_MODE
#define NOCLIP_MODE
*/

// #define SHADING_ENABLED
#define SOUND_ENABLED
// #define TRACK_MEMORY

// max: 1440 by 900
#define INIT_W 900
#define INIT_H 700
#define INIT_MAX_FPS 60
#define INIT_RAY_COLUMN_W 1

// #define INIT_FOV 66.849622365373434718
#define INIT_FOV 90.0
#define INIT_FOV_STEP 0.5
#define INIT_MAX_FOV 100.0

#define INIT_MINIMAP_SCALE 4.0
#define INIT_STOP_DIST 0.2

#define AUDIO_CHUNK_SIZE 1024

#define KEY_TOGGLE_MINIMAP SDL_SCANCODE_1
#define KEY_TOGGLE_CROSSHAIR SDL_SCANCODE_2
#define KEY_TOGGLE_HP_PERCENT SDL_SCANCODE_3

#define KEY_SPEEDUP_1 SDL_SCANCODE_LSHIFT
#define KEY_SPEEDUP_2 SDL_SCANCODE_RSHIFT

#define KEY_JUMP SDL_SCANCODE_SPACE
#define KEY_FLY_UP SDL_SCANCODE_EQUALS // SDL_SCANCODE_O
#define KEY_FLY_DOWN SDL_SCANCODE_MINUS // SDL_SCANCODE_L

#define KEY_USE_WEAPON SDL_BUTTON_LEFT

#define KEY_FORWARD SDL_SCANCODE_W
#define KEY_BACKWARD SDL_SCANCODE_S
#define KEY_LSTRAFE SDL_SCANCODE_A
#define KEY_RSTRAFE SDL_SCANCODE_D

#define PIXEL_FORMAT SDL_PIXELFORMAT_ARGB8888
#define PIXEL_FORMAT_BYTES_PER_PIXEL 4

#define inlinable static inline
#define ASM_I_AM_HERE __asm__ volatile ("nop # I AM HERE")

#define LOOP(max) for (int i = 0; i < max; i++)
#define FAIL(...) {fprintf(stderr, __VA_ARGS__); exit(1);}

#define DEBUG(var, format) printf(#var " = %" #format "\n", var)

#define DEBUG_VEC(vec) printf(#vec " = {%lf, %lf}\n", vec[0], vec[1])
#define DEBUG_IVEC(vec) printf(#vec " = {%d, %d}\n", vec.x, vec.y)

#define DEBUG_RECT(rect) printf(#rect " = {.x = %d, .y = %d, .w = %d, .h = %d}\n", rect.x, rect.y, rect.w, rect.h)
#define DEBUG_FRECT(frect) printf(#frect " = {.x = %lf, .y = %lf, .w = %lf, .h = %lf}\n",\
	(double) frect.x, (double) frect.y, (double) frect.w, (double) frect.h)

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
typedef __m128d vec;
typedef struct {int x, y;} ivec;

/*
https://www.spriters-resource.com/pc_computer/doomdoomii/
https://lodev.org/cgtutor/raycasting4.html
https://medium.com/@Powersaurus/pico-8-raycaster-doors-cd8de9d943b
https://docs.microsoft.com/en-us/cpp/intrinsics/x86-intrinsics-list?view=msvc-160
*/
