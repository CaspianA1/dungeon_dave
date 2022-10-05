#ifndef WEAPON_SPRITE_H
#define WEAPON_SPRITE_H

#include "utils/buffer_defs.h"
#include "animation.h"
#include "rendering/drawable.h"
#include "normal_map_generation.h"
#include "camera.h"
#include "event.h"
#include "rendering/shadow.h"
#include "rendering/entities/skybox.h"
#include "rendering/ambient_occlusion.h"

typedef struct {
	GLfloat cycle_base_time;
	buffer_size_t curr_frame;
	const Animation animation;
} WeaponSpriteAnimationContext;

typedef struct {
	const struct {
		const GLfloat
			frame_width_over_height, size,
			max_movement_magnitude,
			half_movement_cycles_per_sec;
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
update_weapon_sprite_animation, circular_mapping_from_zero_to_one, get_screen_corners,
get_world_corners, get_quad_tbn_matrix, update_uniforms, define_vertex_spec */

WeaponSprite init_weapon_sprite(
	const GLfloat max_yaw_degrees, const GLfloat max_pitch_degrees,
	const GLfloat screen_space_size, const GLfloat secs_per_frame,
	const GLfloat secs_per_movement_cycle,
	const GLfloat max_movement_magnitude,
	const AnimationLayout* const animation_layout,
	const NormalMapConfig* const normal_map_config);

#define deinit_weapon_sprite(ws) deinit_drawable((ws) -> drawable)

void update_weapon_sprite(WeaponSprite* const ws, const Camera* const camera, const Event* const event);
void draw_weapon_sprite_to_shadow_context(const WeaponSprite* const ws);
void draw_weapon_sprite(const WeaponSprite* const ws);

#endif
