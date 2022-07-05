#ifndef EVENT_H
#define EVENT_H

#include "buffer_defs.h"

typedef struct {
	const byte movement_bits; // Forward, backward, left, right, jump, accelerate, click left
	const GLint screen_size[2];

	const GLfloat mouse_movement_percent[2], curr_time_secs, delta_time;
} Event;

Event get_next_event(const Uint32 curr_time_ms, const GLfloat secs_elapsed_between_frames);

#endif
