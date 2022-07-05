#ifndef ANIMATION_C
#define ANIMATION_C

#include "headers/animation.h"

void update_animation_information(
	const GLfloat curr_time_secs, const GLfloat cycle_base_time,
	const Animation animation, buffer_size_t* const texture_id) {

	const GLfloat total_time = curr_time_secs - cycle_base_time;
	const buffer_size_t frames_so_far = (buffer_size_t) ceilf(total_time / animation.secs_for_frame);

	const buffer_size_t texture_id_range = animation.texture_id_range.end - animation.texture_id_range.start;
	*texture_id = (frames_so_far % (texture_id_range + 1)) + animation.texture_id_range.start;
}

#endif
