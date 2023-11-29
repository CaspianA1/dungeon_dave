#include "event.h"
#include "data/constants.h" // For `keys` (not the input parameter), and `milliseconds_per_second`
#include "utils/macro_utils.h" // For `CHECK_BITMASK`

// TODO: avoid all repeated base time logic by making the curr time in secs relative to startup time
Event get_next_event(const Uint32 curr_time_ms, const GLfloat secs_elapsed_between_frames, const Uint8* const keys) {
	GLint viewport_bounds[4];
	glGetIntegerv(GL_VIEWPORT, viewport_bounds);

	const GLint screen_width = viewport_bounds[2], screen_height = viewport_bounds[3];

	int mouse_movement[2]; // Not GLint b/c `SDL_GetRelativeMouseState` is expecting ints

	const bool
		clicking_left = CHECK_BITMASK(SDL_GetRelativeMouseState(mouse_movement, mouse_movement + 1), SDL_BUTTON_LMASK),
		attempting_acceleration = keys[constants.keys.accelerate[0]] || keys[constants.keys.accelerate[1]],
		moving_forward = keys[constants.keys.forward], moving_backward = keys[constants.keys.backward];

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
		.aspect_ratio = (GLfloat) screen_width / screen_height,

		.mouse_movement_percent = {
			(GLfloat) -mouse_movement[0] / screen_width,
			(GLfloat) -mouse_movement[1] / screen_height
		},

		.curr_time_secs = curr_time_ms / constants.milliseconds_per_second,
		.delta_time = secs_elapsed_between_frames,

		.keys = keys
	};
}
