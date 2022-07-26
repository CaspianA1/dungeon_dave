#include "event.h"
#include "constants.h"
#include "utils.h"

Event get_next_event(const Uint32 curr_time_ms, const GLfloat secs_elapsed_between_frames, const Uint8* const keys) {
	static GLint viewport_size[4];
	glGetIntegerv(GL_VIEWPORT, viewport_size);

	const GLint screen_width = viewport_size[2], screen_height = viewport_size[3];

	int mouse_movement[2]; // Not GLint b/c `SDL_GetRelativeMouseState` is expecting ints

	const bool
		attempting_acceleration = keys[constants.keys.accelerate[0]] || keys[constants.keys.accelerate[1]],
		moving_forward = keys[constants.keys.forward], moving_backward = keys[constants.keys.backward],
		clicking_left = CHECK_BITMASK(SDL_GetRelativeMouseState(mouse_movement, mouse_movement + 1), SDL_BUTTON_LMASK);

	// Only accelerating if attempting it, and moving forward or backward exclusively (not both or none)
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

		.screen_size = {screen_width, screen_height},

		.mouse_movement_percent = {
			(GLfloat) -mouse_movement[0] / screen_width,
			(GLfloat) -mouse_movement[1] / screen_height
		},

		.curr_time_secs = curr_time_ms / constants.milliseconds_per_second,
		.delta_time = secs_elapsed_between_frames,

		.keys = keys
	};
}
