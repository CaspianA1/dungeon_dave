#ifndef EVENT_H
#define EVENT_H

#include "buffer_defs.h"

// TODO: store time metadata (current time and delta time) in this

typedef struct {
	const byte movement_bits; // Forward, backward, left, right, jump, accelerate, click left
	const GLint screen_size[2], mouse_movement[2];
} Event;

Event get_next_event(void);

#endif
