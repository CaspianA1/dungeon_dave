#ifndef EVENT_H
#define EVENT_H

#include "utils/buffer_defs.h"

typedef struct {
	const byte movement_bits; // Forward, backward, left, right, jump, accelerate, click left

	const GLint screen_size[2];
	const GLfloat aspect_ratio, mouse_movement_percent[2], curr_time_secs, delta_time;

	const Uint8* const keys;
} Event;

Event get_next_event(const Uint32 curr_time_ms, const GLfloat secs_elapsed_between_frames, const Uint8* const keys);

#endif
