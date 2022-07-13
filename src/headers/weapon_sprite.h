#ifndef WEAPON_SPRITE_H
#define WEAPON_SPRITE_H

#include "buffer_defs.h"
#include "animation.h"
#include "drawable.h"
#include "camera.h"
#include "event.h"
#include "shadow.h"

typedef struct {
	GLfloat cycle_base_time;
	buffer_size_t curr_frame;
	const Animation animation;
} WeaponSpriteAnimationContext;

typedef struct {
	const struct {
		const GLfloat frame_width_over_height, size;
	} screen_space;

	struct {
		const GLfloat max_yaw, max_pitch;
		vec3 corners[corners_per_quad];
	} world_space;
} WeaponSpriteAppearanceContext;

typedef struct {
	const Drawable drawable;
	WeaponSpriteAnimationContext animation_context;
	WeaponSpriteAppearanceContext appearance_context;
} WeaponSprite;

/* Excluded:
update_weapon_sprite_animation, circular_mapping_from_zero_to_one,
get_sway, get_screen_corners_from_sway, get_world_corners_from_screen_corners,
otate_from_camera_movement, update_uniforms */

WeaponSprite init_weapon_sprite(const GLfloat max_yaw_degrees,
	const GLfloat max_pitch_degrees, const GLfloat size,
	const GLfloat texture_rescale_factor, const GLfloat secs_for_frame,
	const AnimationLayout animation_layout);

void deinit_weapon_sprite(const WeaponSprite* const ws);

void update_weapon_sprite(WeaponSprite* const ws, const Camera* const camera, const Event* const event);

void draw_weapon_sprite(
	const WeaponSprite* const ws,
	const vec4* const view_projection,
	const CascadedShadowContext* const shadow_context);

#endif
