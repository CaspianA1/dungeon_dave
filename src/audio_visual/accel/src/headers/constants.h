#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "buffer_defs.h" // For byte and GLfloat
#include <SDL2/SDL_scancode.h>

/* These are defined because M_PI and M_PI_2 are not standard C. They are macros
and not in the `constants` struct b/c other values in that struct depend on them. */
#define TWO_PI 6.28318530717958647692528676655900576f
#define THREE_HALVES_PI 4.71238898038468985769396507491925432f
#define PI 3.14159265358979323846264338327950288f
#define TWO_THIRDS_PI 2.09439510239319562184441962594041856f
#define HALF_PI 1.57079632679489661923132169163975144f
#define FOURTH_PI 0.785398163397448309615660845819875721f

static const struct {
	// This should match the vsync refresh rate, since some of the physics code depends on it
	const byte fps;

	const struct { // All angles are in radians
		const GLfloat eye_height, delta_turn_to_tilt_ratio;
		const struct {const GLfloat fov, hori, vert, tilt;} init;
		// const struct {const GLfloat time_for_full, max;} fov;
		const struct {const GLfloat period, max_amplitude;} pace;
		const struct {const GLfloat vert, tilt, fov;} lims;
		const struct {const GLfloat near, far;} clip_dists;
	} camera;

	const struct {const GLfloat forward_back, additional_forward_back, strafe, xz_decel, g;} accel;

	/* The `look_*` constants indicate the angle that shall be
	turned by for a full mouse cycle across a screen axis */
	const struct {const GLfloat xz_max, jump, look_hori, look_vert;} speeds;

	const struct {
		const SDL_Scancode forward, backward, left, right, jump, accelerate_1, accelerate_2;
	} movement_keys;

} constants = {
	.fps = 60,

	.camera = {
		.eye_height = 0.5f, .delta_turn_to_tilt_ratio = 2.4f,
		.init = {.fov = HALF_PI, .hori = FOURTH_PI, .vert = 0.0f, .tilt = 0.0f},
		.pace = {.period = 0.6f, .max_amplitude = 0.3f},
		.lims = {.vert = HALF_PI, .tilt = 0.25f, .fov = PI / 18.0f}, // Max FOV equals 10 degrees
		.clip_dists = {.near = 0.1f, .far = 441.6729559300637f}
	},

	.accel = {
		.forward_back = 0.2f, .additional_forward_back = 0.2f, .strafe = 0.3f,
		.xz_decel = 0.87f, .g = 13.0f
	},

	.speeds = {.xz_max = 4.0f, .jump = 5.5f, .look_hori = TWO_THIRDS_PI, .look_vert = HALF_PI},

	.movement_keys = {
		.forward = SDL_SCANCODE_W, .backward = SDL_SCANCODE_S, .left = SDL_SCANCODE_A,
		.right = SDL_SCANCODE_D, .jump = SDL_SCANCODE_SPACE, .accelerate_1 = SDL_SCANCODE_LSHIFT,
		.accelerate_2 = SDL_SCANCODE_RSHIFT
	}
};

/* Max world size = 255 by 255 by 255 (with top left corner of block as origin).
So, max look distance in world = sqrt(255 * 255 + 255 * 255 + 255 * 255),
which equals 441.6729559300637 (that is the clip distance) */

#endif
