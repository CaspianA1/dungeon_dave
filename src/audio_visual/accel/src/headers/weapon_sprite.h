#ifndef WEAPON_SPRITE_H
#define WEAPON_SPRITE_H

#include "buffer_defs.h"
#include "animation.h"
#include "camera.h"
#include "event.h"

typedef struct {
	const GLuint texture, shader;
	const Animation animation;
	buffer_size_t curr_frame;

	GLfloat last_frame_time;
	const GLfloat frame_width_over_height, size;
} WeaponSprite;

// Excluded: circular_mapping_from_zero_to_one, update_weapon_sprite

WeaponSprite init_weapon_sprite(const GLfloat size, const GLfloat texture_rescale_factor,
	const GLfloat secs_per_frame, const AnimationSpec animation_spec);

void deinit_weapon_sprite(const WeaponSprite* const ws);
void update_and_draw_weapon_sprite(WeaponSprite* const ws_ref, const Camera* const camera, const Event* const event);

#endif