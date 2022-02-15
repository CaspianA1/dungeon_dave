#ifndef EVENT_H
#define EVENT_H

#include "buffer_defs.h"

typedef struct {
	const byte movement_bits; // Tilt right, tilt left, right, left, backward, forward
	const int screen_size[2], mouse_movement[2];
} Event;

Event get_next_event(void);

#endif
