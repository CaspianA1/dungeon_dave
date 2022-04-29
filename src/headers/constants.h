#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "buffer_defs.h"

/* These are defined because M_PI and M_PI_2 are not standard C. They are macros
and not in the `constants` struct b/c other values in that struct depend on them. */
#define TWO_PI 6.28318530717958647692528676655900576f
#define THREE_HALVES_PI 4.71238898038468985769396507491925432f
#define PI 3.14159265358979323846264338327950288f
#define TWO_THIRDS_PI 2.09439510239319562184441962594041856f
#define HALF_PI 1.57079632679489661923132169163975144f
#define FOURTH_PI 0.785398163397448309615660845819875721f

#define BIT_MOVE_FORWARD 1
#define BIT_MOVE_BACKWARD 2
#define BIT_STRAFE_LEFT 4
#define BIT_STRAFE_RIGHT 8
#define BIT_JUMP 16
#define BIT_ACCELERATE 32

#define BIT_CLICK_LEFT 64
#define BIT_USE_WEAPON BIT_CLICK_LEFT

static const struct {
	const GLchar* const app_name;
	const GLfloat almost_zero;
	const byte max_byte_value, default_fps;

	const struct {
		const GLfloat max_movement_magnitude, time_for_half_movement_cycle;
	} weapon_sprite;

	const struct {
		const struct {const GLint radius; const GLfloat std_dev;} blur;
		const GLfloat intensity;
	} normal_mapping;

	const struct { // All angles are in radians
		const GLfloat near_clip_dist, eye_height, aabb_collision_box_size, tilt_correction_rate, friction;
		const struct {const GLfloat fov, hori, vert, tilt;} init;
		const struct {const GLfloat period, max_amplitude;} pace;
		const struct {const GLfloat vert, tilt, fov;} lims;
	} camera;

	const struct {const GLfloat forward_back, additional_forward_back, strafe, xz_decel, g;} accel;

	/* The `look` constant indicate the angles that shall be turned by
	for a full mouse cycle across a screen axis. [0] = hori, [1] = vert. */
	const struct {const GLfloat xz_max, jump, look[2];} speeds;

	const struct {
		const SDL_Scancode
			forward, backward, left, right,
			jump, accelerate[2], toggle_fullscreen_window,
			ctrl[2], activate_exit[2];
	} keys;

} constants = {
	.app_name = "Dungeon Dave",

	.almost_zero = 0.001f,

	.max_byte_value = 255,
	.default_fps = 60,

	.weapon_sprite = {
		.max_movement_magnitude = 0.2f,
		.time_for_half_movement_cycle = 0.5f
	},

	.normal_mapping = {
		.blur = {.radius = 2, .std_dev = 0.8f},
		.intensity = 1.1f // 0.25f before
	},

	.camera = {
		.near_clip_dist = 0.01f, .eye_height = 0.5f, .aabb_collision_box_size = 0.2f,
		.tilt_correction_rate = 11.0f, .friction = 7.5f,

		.init = {.fov = HALF_PI, .hori = FOURTH_PI, .vert = 0.0f, .tilt = 0.0f},
		.pace = {.period = 0.7f, .max_amplitude = 0.2f},
		.lims = {.vert = HALF_PI, .tilt = 0.15f, .fov = PI / 18.0f} // Max FOV equals 10 degrees
	},

	.accel = {
		.forward_back = 0.15f, .additional_forward_back = 0.05f,
		.strafe = 0.2f, .xz_decel = 0.87f, .g = 13.0f
	},

	.speeds = {.xz_max = 4.0f, .jump = 5.5f, .look = {TWO_THIRDS_PI, HALF_PI}},

	.keys = {
		.forward = SDL_SCANCODE_W, .backward = SDL_SCANCODE_S, .left = SDL_SCANCODE_A,
		.right = SDL_SCANCODE_D, .jump = SDL_SCANCODE_SPACE,
		.accelerate = {SDL_SCANCODE_LSHIFT, SDL_SCANCODE_RSHIFT},
		.toggle_fullscreen_window = SDL_SCANCODE_ESCAPE,
		.ctrl = {SDL_SCANCODE_LCTRL, SDL_SCANCODE_RCTRL},
		.activate_exit = {SDL_SCANCODE_W, SDL_SCANCODE_Q}
	}
};

typedef enum {RefreshRate, AnisotropicFilteringLevel} RuntimeConstantName;

GLfloat get_runtime_constant(const RuntimeConstantName runtime_constant_name);

#endif
