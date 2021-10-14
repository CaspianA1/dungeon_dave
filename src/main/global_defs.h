#ifdef __clang__

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>

#else

#include "/usr/local/include/SDL2/SDL.h"
#include "/usr/local/include/SDL2/SDL_ttf.h"
#include "/usr/local/include/SDL2/SDL_mixer.h"

#endif

// #define PLANAR_MODE
// #define NOCLIP_MODE

// #define ANTIALIASED_MIPMAPPING
// #define DISABLE_ENEMIES
#define SHADING_ENABLED
// #define PERLIN_SHADING
// #define SOUND_ENABLED
// #define TRACK_MEMORY

#define FLOORCAST_THREADS 3

// max: 1440 by 900; and 960 by 540 is 1920 by 1080 ratio
#define INIT_W 800
#define INIT_H 600
#define INIT_MAX_FPS 60
#define INIT_RAY_COLUMN_W 1

// #define INIT_FOV 66.849622365373434718
#define INIT_FOV 110.0
#define INIT_FOV_STEP 1.0
#define INIT_MAX_FOV 120.0

#define INIT_MINIMAP_SCALE 4.8
#define INIT_HP 30.0

#define AUDIO_CHUNK_SIZE 512 // 1024 before

#define INIT_BEGIN_LEVEL_FADE 100

#define KEY_TOGGLE_OPTIONS_MENU SDL_SCANCODE_ESCAPE
#define KEY_TOGGLE_MINIMAP SDL_SCANCODE_1
#define KEY_TOGGLE_HP_PERCENT SDL_SCANCODE_2
#define KEY_TOGGLE_CROSSHAIR SDL_SCANCODE_3

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
#define PIXEL_FORMAT_DEPTH 32
#define WINDOW_RENDERER_FLAGS SDL_RENDERER_ACCELERATED | SDL_WINDOW_RESIZABLE

#define inlinable static inline

// used for finding a code snippet in an output assembly file
#define ASM_COMMENT(msg) __asm__ __volatile__ ("nop # " msg)
#define STARTUP_LOG(activity) puts("Initializing " activity)
#define LOOP(max) for (int i = 0; i < max; i++)
#define FAIL(...) {fprintf(stderr, __VA_ARGS__); exit(1);}

#define DEBUG(var, format) printf(#var " = %" #format "\n", var)

#define DEBUG_BYTE(byte)\
	for (int i = 7; i >= 0; i--) putchar((byte & (1 << i)) ? '1' : '0');\
	putchar('\n');

#define DEBUG_VEC(vec) printf(#vec " = {%lf, %lf}\n", vec[0], vec[1])
#define DEBUG_IVEC(vec) printf(#vec " = {%d, %d}\n", vec.x, vec.y)

#define DEBUG_RECT(rect) printf(#rect " = {.x = %d, .y = %d, .w = %d, .h = %d}\n", rect.x, rect.y, rect.w, rect.h)
#define DEBUG_FRECT(frect) printf(#frect " = {.x = %lf, .y = %lf, .w = %lf, .h = %lf}\n",\
	(double) frect.x, (double) frect.y, (double) frect.w, (double) frect.h)

//////////

typedef uint_fast8_t byte;
typedef __m128d vec;
typedef __m128 vec3D; // 4 32-bit floats instead of 2 64-bit floats (one extra float)
typedef struct {int x, y;} ivec;

enum {enemy_count = 2};

const double
	two_pi = M_PI * 2.0,
	half_pi = M_PI / 2.0,
	three_pi_over_two = 3.0 * M_PI / 2.0,
	five_pi_over_two = 5.0 * M_PI / 2.0,
	g = -9.8,
	almost_zero = 0.001,
	almost_almost_zero = 0.000000001,
	actor_eye_height = 0.5,
	actor_height = 1.0,
	enemy_dist_for_attack = 1.0;

static const byte bitmasks[5] = {1 << 0, 1 << 1, 1 << 2, 1 << 3, 1 << 4};

#define mask_forward_or_backward_movement bitmasks[0]
#define mask_forward_movement bitmasks[1]
#define mask_backward_movement bitmasks[2]

#define mask_in_use_weapon bitmasks[0]
#define mask_short_range_weapon bitmasks[1]
#define mask_spawns_projectile_weapon bitmasks[2]
#define mask_paces_sideways_weapon bitmasks[3]
#define mask_recently_used_weapon bitmasks[4]

#define mask_recently_attacked_enemy bitmasks[0]
#define mask_long_range_attack_enemy bitmasks[1]

#define mask_can_move_through_thing bitmasks[0]
#define mask_skip_rendering_thing bitmasks[1]

#define mask_currently_jumping bitmasks[0]
#define mask_made_noise_jump bitmasks[1]

/*
https://www.spriters-resource.com/pc_computer/doomdoomii/
https://lodev.org/cgtutor/raycasting4.html
https://medium.com/@Powersaurus/pico-8-raycaster-doors-cd8de9d943b
https://docs.microsoft.com/en-us/cpp/intrinsics/x86-intrinsics-list?view=msvc-160
*/
