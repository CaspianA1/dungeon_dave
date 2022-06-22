#ifndef WEAPON_SPRITE_H
#define WEAPON_SPRITE_H

#include "buffer_defs.h"
#include "animation.h"
#include "camera.h"
#include "event.h"
#include "csm.h"

typedef struct {
	const GLuint shader, texture;
	const Animation animation;

	Uint32 cycle_base_time;
	buffer_size_t curr_frame;

	const GLfloat frame_width_over_height, size, max_yaw, max_pitch;
} WeaponSprite;

/* Excluded: update_weapon_sprite_animation, circular_mapping_from_zero_to_one, get_sway,
get_screen_corners_from_sway, get_world_corners_from_screen_corners, rotate_from_camera_movement */

WeaponSprite init_weapon_sprite(const GLfloat max_yaw_degrees,
	const GLfloat max_pitch_degrees, const GLfloat size,
	const GLfloat texture_rescale_factor, const GLfloat secs_for_frame,
	const AnimationLayout animation_layout);

void deinit_weapon_sprite(const WeaponSprite* const ws);

void update_and_draw_weapon_sprite(WeaponSprite* const ws, const Camera* const camera,
	const Event* const event, const CascadedShadowContext* const shadow_context);

#endif
