#ifndef EVENT_H
#define EVENT_H

#include "buffer_defs.h"

typedef struct {
	const byte movement_bits; // Forward, backward, left, right, jump, accelerate, click left
	const GLint screen_size[2];
	const GLfloat mouse_movement_percent[2], curr_time_secs;
} Event;

Event get_next_event(void);

#endif
