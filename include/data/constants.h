#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "utils/typedefs.h" // For OpenGL types + `byte`
#include "utils/sdl_include.h" // For `SDL_Scancode`
#include "utils/cglm_include.h" // For pi variants + `vec2`

static const GLfloat
	TWO_PI = GLM_PIf * 2.0f,
	TWO_THIRDS_PI = GLM_PIf * 2.0f / 3.0f,
	THREE_HALVES_PI = GLM_PIf * 3.0f / 2.0f;

//////////

#define BIT_MOVE_FORWARD 1
#define BIT_MOVE_BACKWARD 2
#define BIT_STRAFE_LEFT 4
#define BIT_STRAFE_RIGHT 8
#define BIT_JUMP 16
#define BIT_ACCELERATE 32

#define BIT_CLICK_LEFT 64
#define BIT_USE_WEAPON BIT_CLICK_LEFT

//////////

#define KEY_FLY SDL_SCANCODE_1
#define KEY_TOGGLE_WIREFRAME_MODE SDL_SCANCODE_2
#define KEY_PRINT_POSITION SDL_SCANCODE_3
#define KEY_PRINT_DIRECTION SDL_SCANCODE_4
#define KEY_PRINT_OPENGL_ERROR SDL_SCANCODE_5
#define KEY_PRINT_SDL_ERROR SDL_SCANCODE_6

////////// TODO: put these in the struct below

// #define TRACK_MEMORY
// #define PRINT_SHADER_VALIDATION_LOG

//////////

enum { // `enum` is used to make these values compile-time constants
	components_per_face_vertex_pos = 3,
	components_per_face_vertex = 4,
	vertices_per_face = 6,

	vertices_per_triangle = 3,
	corners_per_quad = 4,
	corners_per_frustum = 8,
	planes_per_frustum = 6,
	faces_per_cubemap = 6,
	vertices_per_skybox = 14,

	num_unique_object_types = 3 // Sector face, billboard, and weapon sprite
};

//////////

static const struct {
	const GLfloat milliseconds_per_second;
	const byte max_byte_value;

	const struct { // All angles are in radians
		const GLfloat near_clip_dist, eye_height, aabb_collision_box_size, tilt_correction_rate, friction, init_fov;
		const struct {const GLfloat period, max_amplitude;} pace;
		const struct {const GLfloat fov_change, hori_wrap_around, vert_max, tilt_max;} limits;
	} camera;

	const struct {const GLfloat forward_back, additional_forward_back, strafe, g;} accel;

	/* The `look` constant indicate the angles that shall be turned by
	for a full mouse cycle across a screen axis. [0] = hori, [1] = vert. */
	const struct {
		const GLfloat xz_max, jump;
		const vec2 look;
	} speeds;

	const struct {
		const SDL_Scancode
			forward, backward, left, right,
			jump, accelerate[2], toggle_fullscreen_window,
			ctrl[2], activate_exit[2];
	} keys;

} constants = {
	.milliseconds_per_second = 1000.0f,
	.max_byte_value = 255,

	.camera = {
		.near_clip_dist = 0.25f, .eye_height = 0.5f, .aabb_collision_box_size = 0.2f,
		.tilt_correction_rate = 11.0f, .friction = 7.5f,
		.init_fov = GLM_PI_2f,

		.pace = {.period = 0.7f, .max_amplitude = 0.2f},
		.limits = {.fov_change = GLM_PIf / 18.0f, .hori_wrap_around = TWO_PI, .vert_max = GLM_PI_2f, .tilt_max = 0.2f}
	},

	.accel = {
		.forward_back = 6.0f,
		.additional_forward_back = 2.5f,
		.strafe = 5.5f, .g = 13.0f
	},

	.speeds = {.xz_max = 4.0f, .jump = 5.5f, .look = {TWO_THIRDS_PI, GLM_PI_2f}},

	.keys = {
		.forward = SDL_SCANCODE_W, .backward = SDL_SCANCODE_S, .left = SDL_SCANCODE_A,
		.right = SDL_SCANCODE_D, .jump = SDL_SCANCODE_SPACE,
		.accelerate = {SDL_SCANCODE_LSHIFT, SDL_SCANCODE_RSHIFT},
		.toggle_fullscreen_window = SDL_SCANCODE_ESCAPE,
		.ctrl = {SDL_SCANCODE_LCTRL, SDL_SCANCODE_RCTRL},
		.activate_exit = {SDL_SCANCODE_W, SDL_SCANCODE_Q}
	}
};

#endif
