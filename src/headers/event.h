#ifndef EVENT_H
#define EVENT_H

#include "buffer_defs.h"

/* TODO:
- Store time metadata (current time and delta time) in this
- Remove time metadata stored in other structs after that
- Then, all events will depend on the same time every tick
*/

typedef struct {
	const byte movement_bits; // Forward, backward, left, right, jump, accelerate, click left
	const GLint screen_size[2];
	const GLfloat mouse_movement_percent[2];
} Event;

Event get_next_event(void);

#endif
