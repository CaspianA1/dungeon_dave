#ifndef WEAPON_SPRITE_C
#define WEAPON_SPRITE_C

#include "headers/weapon_sprite.h"
#include "headers/constants.h"
#include "headers/texture.h"
#include "headers/shaders.h"

WeaponSprite init_weapon_sprite(const GLfloat size, const GLfloat texture_rescale_factor,
	const GLfloat secs_per_frame, const AnimationLayout animation_layout) {

	/* It's a bit wasteful to load the surface in `init_texture_set`
	and here, but this makes the code much more readable. TODO: perhaps
	query data about the texture set to figure out the frame size. */

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
		.shader = init_shader(weapon_vertex_shader, weapon_fragment_shader),

		.animation = {
			.texture_id_range = {.start = 0, .end = (buffer_size_t) animation_layout.total_frames},
			.secs_per_frame = secs_per_frame
		},

		.curr_frame = 0, .last_frame_time = SDL_GetTicks() / 1000.0f,
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

static void update_weapon_sprite(WeaponSprite* const ws, const Event* const event) {
	buffer_size_t curr_frame = ws -> curr_frame;

	if (curr_frame == 0) {
		if (CHECK_BITMASK(event -> movement_bits, BIT_USE_WEAPON)) curr_frame++;
	}
	else {
		update_animation_information(
			&ws -> last_frame_time, &curr_frame,
			ws -> animation, SDL_GetTicks() / 1000.0f);
	}

	ws -> curr_frame = curr_frame;
}

void update_and_draw_weapon_sprite(WeaponSprite* const ws_ref, const Camera* const camera, const Event* const event) {
	update_weapon_sprite(ws_ref, event);

	const WeaponSprite ws = *ws_ref;

	use_shader(ws.shader);
	static GLint weapon_corners_id, frame_index_id;

	ON_FIRST_CALL(
		INIT_UNIFORM(weapon_corners, ws.shader);
		INIT_UNIFORM(frame_index, ws.shader);
		use_texture(ws.texture, ws.shader, "frame_sampler", TexSet, WEAPON_TEXTURE_UNIT);
	);

	const GLfloat
		inverse_screen_aspect_ratio = (GLfloat) event -> screen_size[1] / event -> screen_size[0],
		smooth_speed_xz_percent = circular_mapping_from_zero_to_one(camera -> speed_xz_percent);

	const GLfloat
		time_pace = sinf((SDL_GetTicks() / 1000.0f) * PI / constants.weapon_sprite.time_for_half_movement_cycle),
		weapon_movement_magnitude = constants.weapon_sprite.max_movement_magnitude * smooth_speed_xz_percent;

	const GLfloat across = time_pace * weapon_movement_magnitude * 0.5f * smooth_speed_xz_percent; // From -magnitude / 2.0f to magnitude / 2
	const GLfloat down = (fabsf(across) - weapon_movement_magnitude) * smooth_speed_xz_percent; // From 0.0f to -magnitude

	//////////

	vec2 weapon_corners[4];

	for (byte i = 0; i < 4; i++) {
		const signed char screen_corner_x = (i & 1) ? 1 : -1, screen_corner_y = (i > 1) ? 1 : -1;

		GLfloat* const weapon_corner = weapon_corners[i];
		weapon_corner[0] = screen_corner_x * ws.size * ws.frame_width_over_height * inverse_screen_aspect_ratio + across;
		weapon_corner[1] = screen_corner_y * ws.size + (ws.size - 1.0f) + down;
	}

	//////////

	UPDATE_UNIFORM(weapon_corners, 2fv, 4, (GLfloat*) weapon_corners);
	UPDATE_UNIFORM(frame_index, 1ui, ws.curr_frame);

	WITH_BINARY_RENDER_STATE(GL_BLEND,
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, corners_per_quad);
	);
}

#endif
