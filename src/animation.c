#include "animation.h"

bool update_animation_information(
	const GLfloat curr_time_secs, const Animation animation,
	GLfloat* const cycle_start_time, texture_id_t* const texture_id) {

	const GLfloat time_since_cycle_start = curr_time_secs - *cycle_start_time;

	texture_id_t new_texture_id = animation.texture_id_range.start +
		(texture_id_t) (time_since_cycle_start / animation.secs_for_frame);

	const bool just_finished_cycle = (new_texture_id > animation.texture_id_range.end);

	if (just_finished_cycle) {
		new_texture_id = animation.texture_id_range.start;
		*cycle_start_time = curr_time_secs;
	}

	*texture_id = new_texture_id;

	return just_finished_cycle;
}
