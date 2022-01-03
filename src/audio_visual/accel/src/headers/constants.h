#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <SDL2/SDL_scancode.h>
#include <GL/glew.h> // For GLFloat

/* These are defined because M_PI and M_PI_2 are not standard C. They are macros
and not in the `constants` struct b/c other values in that struct depend on them. */
#define PI 3.14159265358979323846264338327950288f
#define HALF_PI 1.57079632679489661923132169163975144f
#define FOURTH_PI 0.785398163397448309615660845819875721f

static const struct {
	const byte fps;

	const struct { // All angles are in radians
		const GLfloat eye_height;
		const struct {const GLfloat fov, hori, vert, tilt;} init;
		const struct {const GLfloat vert, tilt;} lims;
		const struct {const GLfloat near, far;} clip_dists;
	} camera;

	const struct {const GLfloat forward_back, strafe, xz_decel, g;} accel;
	const struct {const GLfloat y_jump, xz_max, look, tilt;} speeds;

	const struct {
		const SDL_Scancode forward, backward, left, right, tilt_left, tilt_right, jump;
	} movement_keys;

} constants = {
	.fps = 60,

	.camera = {
		.eye_height = 0.5f,
		.init = {.fov = HALF_PI, .hori = FOURTH_PI, .vert = 0.0f, .tilt = 0.0f},
		.lims = {.vert = HALF_PI, .tilt = HALF_PI},
		.clip_dists = {0.1f, 441.6729559300637f}
	},

	.accel = {.forward_back = 0.38f, .strafe = 0.4f, .xz_decel = 0.87f, .g = 13.0f},
	.speeds = {.y_jump = 5.5f, .xz_max = 4.0f, .look = 0.08f, .tilt = 1.0f},

	.movement_keys = {
		SDL_SCANCODE_W, SDL_SCANCODE_S, SDL_SCANCODE_A,
		SDL_SCANCODE_D, SDL_SCANCODE_T, SDL_SCANCODE_Y, SDL_SCANCODE_SPACE
	}
};

/* Max world size = 255 by 255 by 255 (with top left corner of block as origin).
So, max look distance in world = sqrt(255 * 255 + 255 * 255 + 255 * 255),
which equals 441.6729559300637 (that is the clip distance) */

#endif
