#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "utils/typedefs.h" // For OpenGL types + `byte`
#include "cglm/cglm.h" // For pi variants + `vec2`
#include "utils/sdl_include.h" // For `SDL_Scancode`

static const GLfloat
	PI = GLM_PIf,
	TWO_PI = GLM_PIf * 2.0f,
	TWO_THIRDS_PI = GLM_PIf * 2.0f / 3.0f,
	THREE_HALVES_PI = GLM_PIf * 3.0f / 2.0f,
	HALF_PI = GLM_PI_2f,
	ONE_FOURTH_PI = GLM_PI_4f;

//////////

#define BIT_MOVE_FORWARD 1
#define BIT_MOVE_BACKWARD 2
#define BIT_STRAFE_LEFT 4
#define BIT_STRAFE_RIGHT 8
#define BIT_JUMP 16
#define BIT_ACCELERATE 32

#define BIT_CLICK_LEFT 64
#define BIT_USE_WEAPON BIT_CLICK_LEFT

////////// TODO: put these in the struct below

// #define TRACK_MEMORY
// #define DEBUG_AO_MAP_GENERATION
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

	num_unique_object_types = 3 // Sector face, billboard, and weapon sprite
};

//////////

static const struct {
	const GLenum default_depth_func;
	const GLfloat list_realloc_rate, milliseconds_per_second, one_over_max_byte_value;
	const byte max_byte_value, min_shadow_map_cascades, skybox_sphere_fineness; // Note: the fineness must never equal 0

	const struct { // All angles are in radians
		const GLfloat near_clip_dist, eye_height, aabb_collision_box_size, tilt_correction_rate, init_fov;
		const struct {const GLfloat floor, wall;} frictions;
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
	.default_depth_func = GL_LEQUAL,

	.list_realloc_rate = 2.0f,
	.milliseconds_per_second = 1000.0f,
	.one_over_max_byte_value = 1.0f / 255.0f,
	.max_byte_value = 255,

	.min_shadow_map_cascades = 3,
	.skybox_sphere_fineness = 80,

	.camera = {
		.near_clip_dist = 0.25f, .eye_height = 0.5f, .aabb_collision_box_size = 0.2f,
		.tilt_correction_rate = 11.0f, .init_fov = HALF_PI,

		.frictions = {.floor = 7.5f, .wall = 6.0f},
		.pace = {.period = 0.7f, .max_amplitude = 0.2f},
		.limits = {.fov_change = PI / 18.0f, .hori_wrap_around = TWO_PI, .vert_max = HALF_PI, .tilt_max = 0.2f}
	},

	.accel = {
		.forward_back = 6.0f,
		.additional_forward_back = 2.5f,
		.strafe = 5.5f, .g = 13.0f
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

#endif
