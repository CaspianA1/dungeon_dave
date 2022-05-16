#ifndef ANIMATION_C
#define ANIMATION_C

#include "headers/animation.h"

void update_animation_information(const Uint32 cycle_base_time, buffer_size_t* const texture_id, const Animation animation) {
	const GLfloat total_time = (SDL_GetTicks() - cycle_base_time) / 1000.0f;
	const buffer_size_t frames_so_far = (buffer_size_t) roundf(total_time / animation.secs_for_frame);

	const buffer_size_t texture_id_range = animation.texture_id_range.end - animation.texture_id_range.start;
	*texture_id = (frames_so_far % (texture_id_range + 1)) + animation.texture_id_range.start;
}

#endif
