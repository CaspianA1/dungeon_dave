#ifndef WEAPON_SPRITE_H
#define WEAPON_SPRITE_H

#include "buffer_defs.h"
#include "animation.h"
#include "camera.h"
#include "event.h"
#include "csm.h"

/* TODO: contain the world-space weapon sprite coordinates in a vbo, and
define a vao (so that the weapon can cast a shadow later). But make a
Drawable struct first, or something along those lines. */

typedef struct {
	const GLuint texture, shader;
	const Animation animation;

	Uint32 cycle_base_time;
	buffer_size_t curr_frame;

	const GLfloat frame_width_over_height, size;
} WeaponSprite;

// Excluded: circular_mapping_from_zero_to_one, update_weapon_sprite_animation

WeaponSprite init_weapon_sprite(const GLfloat size, const GLfloat texture_rescale_factor,
	const GLfloat secs_for_frame, const AnimationLayout animation_layout);

void deinit_weapon_sprite(const WeaponSprite* const ws);

void update_and_draw_weapon_sprite(WeaponSprite* const ws_ref, const Camera* const camera,
	const Event* const event, const CascadedShadowContext* const shadow_context, const mat4 model_view_projection);

#endif
