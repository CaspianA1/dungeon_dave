#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "utils/buffer_defs.h"

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

typedef struct { // These are the Euler angles + FOV
	GLfloat fov, hori, vert, tilt;
} Angles;

typedef struct {
	const GLfloat fov_change, hori_wrap_around, vert_max, tilt_max;
} AngleLimits;

static const struct {
	const GLfloat milliseconds_per_second;
	const byte max_byte_value;

	const struct {
		const GLchar* const app_name;

		const byte
			opengl_major_minor_version[2],
			default_fps, depth_buffer_bits;

		const GLint size[2];
	} window;

	const struct {
		const byte aniso_filtering_level, multisample_samples;

		const struct {
			const bool enabled;
			const GLfloat min_layers, max_layers, height_scale, lod_cutoff;
		} parallax_mapping;

		const struct {const GLfloat diffuse, normal;} bilinear_percents;
		const struct {const GLfloat ambient, diffuse, specular;} strengths;
		const struct {const GLfloat matte, rough;} specular_exponents;

		const struct {
			const GLuint sample_radius, esm_exponent;
			const GLfloat esm_exponent_layer_scale_factor;
		} shadow_mapping;

		const vec3 overall_scene_tone;
		const GLfloat specular_exponent, tone_mapping_max_white, noise_granularity;
	} lighting;

	const struct { // All angles are in radians
		const GLfloat near_clip_dist, eye_height, aabb_collision_box_size, tilt_correction_rate, friction;
		const struct {const GLfloat period, max_amplitude;} pace;

		const Angles init;
		const AngleLimits limits;
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
	.milliseconds_per_second = 1000.0f,
	.max_byte_value = 255,

	.window = {
		.app_name = "Dungeon Dave",
		.opengl_major_minor_version = {4, 0},
		.default_fps = 60, .depth_buffer_bits = 24,
		.size = {800, 600}
	},

	// TODO: remove this from this struct in some way
	.lighting = {
		.aniso_filtering_level = 8, .multisample_samples = 4,

		.parallax_mapping = {
			.enabled = true,
			.min_layers = 4.0f, .max_layers = 32.0f,
			.height_scale = 0.02f, .lod_cutoff = 1.5f
		},

		.bilinear_percents = {.diffuse = 0.75f, .normal = 0.9f},
		.strengths = {.ambient = 0.7f, .diffuse = 0.6f, .specular = 0.6f},
		.specular_exponents = {.matte = 8.0f, .rough = 128.0f},

		.shadow_mapping = {
			.sample_radius = 2, .esm_exponent = 45,
			.esm_exponent_layer_scale_factor = 1.8f
		},

		.overall_scene_tone = {0.968627f, 0.878431f, 0.823529f},
		.tone_mapping_max_white = 1.5f, .noise_granularity = 0.001f
	},

	.camera = {
		.near_clip_dist = 0.25f, .eye_height = 0.5f, .aabb_collision_box_size = 0.2f,
		.tilt_correction_rate = 11.0f, .friction = 7.5f,

		.pace = {.period = 0.7f, .max_amplitude = 0.2f},
		.init = {.fov = HALF_PI, .hori = FOURTH_PI, .vert = 0.0f, .tilt = 0.0f},
		.limits = {.fov_change = PI / 18.0f, .hori_wrap_around = TWO_PI, .vert_max = HALF_PI, .tilt_max = 0.2f}
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

//////////

typedef enum {
	RefreshRate,
	AnisotropicFilteringLevel
} RuntimeConstantName;

GLfloat get_runtime_constant(const RuntimeConstantName runtime_constant_name);

#endif
