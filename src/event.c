#ifndef EVENT_C
#define EVENT_C

#include "headers/constants.h"
#include "headers/event.h"
#include "headers/utils.h"

Event get_next_event(void) {
	static GLint viewport_size[4];
	glGetIntegerv(GL_VIEWPORT, viewport_size);

	int mouse_movement[2]; // Not GLint b/c `SDL_GetRelativeMouseState` is expecting ints

	const bool
		attempting_acceleration = keys[constants.keys.accelerate[0]] || keys[constants.keys.accelerate[1]],
		moving_forward = keys[constants.keys.forward], moving_backward = keys[constants.keys.backward],
		clicking_left = CHECK_BITMASK(SDL_GetRelativeMouseState(mouse_movement, mouse_movement + 1), SDL_BUTTON_LMASK);

	const bool accelerating = attempting_acceleration && (moving_forward ^ moving_backward);

	return (Event) {
		.movement_bits = (byte) (
			moving_forward |
			(moving_backward << 1) |
			(keys[constants.keys.left] << 2) |
			(keys[constants.keys.right] << 3) |
			(keys[constants.keys.jump] << 4) |
			(accelerating << 5) |
			(clicking_left << 6)
		),

		.screen_size = {viewport_size[2], viewport_size[3]},
		.mouse_movement = {mouse_movement[0], mouse_movement[1]}
	};
}

#endif
