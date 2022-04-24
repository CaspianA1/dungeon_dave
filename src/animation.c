#ifndef ANIMATION_C
#define ANIMATION_C

#include "headers/animation.h"

void update_animation_information(GLfloat* const last_frame_time,
	buffer_size_t* const texture_id, const Animation animation, const GLfloat curr_time) {

	const GLfloat time_delta = curr_time - *last_frame_time;

	if (time_delta >= animation.secs_per_frame) {
		if ((*texture_id)++ >= animation.texture_id_range.end)
			*texture_id = animation.texture_id_range.start;

		*last_frame_time = curr_time;
	}
}

#endif
