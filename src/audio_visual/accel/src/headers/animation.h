#ifndef ANIMATION_H
#define ANIMATION_H

#include "buffer_defs.h"

typedef struct {
	const struct {const buffer_size_t start, end;} texture_id_range;
	const GLfloat secs_per_frame;
} Animation;

void update_animation_information(GLfloat* const last_frame_time,
	buffer_size_t* const texture_id, const Animation animation, const GLfloat curr_time);

#endif