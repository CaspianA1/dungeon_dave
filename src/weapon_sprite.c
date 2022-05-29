#ifndef WEAPON_SPRITE_C
#define WEAPON_SPRITE_C

#include "headers/weapon_sprite.h"
#include "headers/constants.h"
#include "headers/texture.h"
#include "headers/shader.h"

WeaponSprite init_weapon_sprite(const GLfloat size, const GLfloat texture_rescale_factor,
	const GLfloat secs_for_frame, const AnimationLayout animation_layout) {

	/* It's a bit wasteful to load the surface in `init_texture_set`
	and here, but this makes the code much more readable. TODO: perhaps
	query data about the texture set to figure out the frame size, if possible. */

	SDL_Surface* const peek_surface = init_surface(animation_layout.spritesheet_path);

	const GLsizei frame_size[2] = {
		peek_surface -> w / animation_layout.frames_across,
		peek_surface -> h / animation_layout.frames_down
	};

	deinit_surface(peek_surface);

	return (WeaponSprite) {
		.texture = init_texture_set(TexNonRepeating,
			OPENGL_WEAPON_MAG_FILTER, OPENGL_WEAPON_MIN_FILTER, 0, 1,
			(GLsizei) (frame_size[0] * texture_rescale_factor),
			(GLsizei) (frame_size[1] * texture_rescale_factor),
			NULL, &animation_layout
		),

		// TODO: for multiple weapons, share this shader
		.shader = init_shader("assets/shaders/weapon.vert", NULL, "assets/shaders/weapon.frag"),

		.animation = {
			.texture_id_range = {.start = 0, .end = (buffer_size_t) animation_layout.total_frames},
			.secs_for_frame = secs_for_frame
		},

		.cycle_base_time = 0, .curr_frame = 0,
		.frame_width_over_height = (GLfloat) frame_size[0] / frame_size[1],
		.size = size
	};
}

void deinit_weapon_sprite(const WeaponSprite* const ws) {
	deinit_texture(ws -> texture);
	deinit_shader(ws -> shader);
}

// Given an input between 0 and 1, this returns the y-value of the top left side of a circle
static GLfloat circular_mapping_from_zero_to_one(const GLfloat x) {
	/*
	- Goal: a smooth mapping from 0 to 1
	- And the y-value of a circle from x = 0 to 1, when its center equals (1, 0), achieves just that

	Circle equation: x^2 + y^2 = 1
	Shifted 1 unit to the right: (x - 1)^2 + y^2 = 1
	y^2 = 1 - (x - 1)^2
	y = sqrt(1 - (x - 1)^2) */

	const GLfloat x_minus_one = x - 1.0f;
	return sqrtf(1.0f - x_minus_one * x_minus_one);
}

static void update_weapon_sprite_animation(WeaponSprite* const ws, const Event* const event) {
	buffer_size_t curr_frame = ws -> curr_frame;

	if (curr_frame == 0) {
		if (CHECK_BITMASK(event -> movement_bits, BIT_USE_WEAPON)) {
			ws -> cycle_base_time = SDL_GetTicks();
			curr_frame++;
		}
	}
	else update_animation_information(ws -> cycle_base_time, &curr_frame, ws -> animation);

	ws -> curr_frame = curr_frame;
}

void update_and_draw_weapon_sprite(WeaponSprite* const ws_ref, const Camera* const camera,
	const Event* const event, const ShadowMapContext* const shadow_map_context,
	const mat4 model_view_projection) {

	update_weapon_sprite_animation(ws_ref, event);

	const WeaponSprite ws = *ws_ref;

	use_shader(ws.shader);
	static GLint frame_index_id, screen_corners_id, world_corners_id;

	ON_FIRST_CALL(
		INIT_UNIFORM(frame_index, ws.shader);
		INIT_UNIFORM(screen_corners, ws.shader);
		INIT_UNIFORM(world_corners, ws.shader);

		INIT_UNIFORM_VALUE(ambient, ws.shader, 1f, constants.lighting.ambient);
		INIT_UNIFORM_VALUE(pcf_radius, ws.shader, 1i, constants.lighting.pcf_radius);
		INIT_UNIFORM_VALUE(esm_constant, ws.shader, 1f, constants.lighting.esm_constant);
		INIT_UNIFORM_VALUE(biased_light_model_view_projection, ws.shader, Matrix4fv,
			1, GL_FALSE, &shadow_map_context -> light.biased_model_view_projection[0][0]);

		use_texture(ws.texture, ws.shader, "frame_sampler", TexSet, WEAPON_TEXTURE_UNIT);
		use_texture(shadow_map_context -> buffers.depth_texture, ws.shader, "shadow_map_sampler",
			TexPlain, SHADOW_MAP_TEXTURE_UNIT);
	);

	const GLfloat
		inverse_screen_aspect_ratio = (GLfloat) event -> screen_size[1] / event -> screen_size[0],
		smooth_speed_xz_percent = circular_mapping_from_zero_to_one(camera -> speed_xz_percent);

	const GLfloat
		time_pace = sinf((SDL_GetTicks() / 1000.0f) * PI / constants.weapon_sprite.time_for_half_movement_cycle),
		weapon_movement_magnitude = constants.weapon_sprite.max_movement_magnitude * smooth_speed_xz_percent;

	const GLfloat across = time_pace * weapon_movement_magnitude * 0.5f * smooth_speed_xz_percent; // From -magnitude / 2.0f to magnitude / 2.0f
	const GLfloat down = (fabsf(across) - weapon_movement_magnitude) * smooth_speed_xz_percent; // From 0.0f to -magnitude

	////////// Screen corner determination

	vec2 screen_corners[4];

	const GLfloat
		across_term = ws.size * ws.frame_width_over_height * inverse_screen_aspect_ratio,
		down_term = (ws.size - 1.0f) + down;

	screen_corners[0][0] = screen_corners[2][0] = across - across_term;
	screen_corners[1][0] = screen_corners[3][0] = across + across_term;
	screen_corners[0][1] = screen_corners[1][1] = down_term - ws.size;
	screen_corners[2][1] = screen_corners[3][1] = down_term + ws.size;

	////////// World corner determination

	vec3 world_corners[4];
	mat4 inv_model_view_projection;
	glm_mat4_inv((vec4*) model_view_projection, inv_model_view_projection);
	const vec4 viewport = {-1.0f, -1.0f, 1.0f, 1.0f};

	for (byte i = 0; i < 4; i++) {
		const GLfloat* const screen_corner = screen_corners[i];

		glm_unprojecti((vec3) {screen_corner[0], screen_corner[1], constants.weapon_sprite.ndc_dist_from_camera},
			(vec4*) inv_model_view_projection, (GLfloat*) viewport, world_corners[i]);
	}

	//////////

	UPDATE_UNIFORM(frame_index, 1ui, ws.curr_frame);
	UPDATE_UNIFORM(screen_corners, 2fv, 4, (GLfloat*) screen_corners);
	UPDATE_UNIFORM(world_corners, 3fv, 4, (GLfloat*) world_corners);

	WITH_BINARY_RENDER_STATE(GL_BLEND,
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, corners_per_quad);
	);
}

#endif
