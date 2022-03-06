#ifndef EVENT_H
#define EVENT_H

#include "buffer_defs.h"

typedef struct {
	const byte movement_bits; // Forward, backward, left, right, jump, accelerate, click left
	const int screen_size[2], mouse_movement[2];
} Event;

Event get_next_event(void);

#endif
