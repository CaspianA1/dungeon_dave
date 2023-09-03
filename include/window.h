#ifndef WINDOW_H
#define WINDOW_H

#include <stdbool.h> // For `bool`
#include "event.h" // For `Event`

typedef struct {
	const char* const app_name;

	const struct {
		const bool
			vsync, aniso_filtering,
			multisampling, software_renderer;
	} enabled;

	const byte
		aniso_filtering_level, multisample_samples,
		default_fps, depth_buffer_bits, opengl_major_minor_version[2];

	const uint16_t window_size[2];
} WindowConfig;

// Excluded: init_screen, deinit_screen, resize_window_if_needed, application_should_exit, loop_application

typedef bool (*const drawer_t) (void* const, const Event* const, const WindowConfig* const);

void make_application(
	const WindowConfig* const config,
	void* (*const init) (const WindowConfig* const),
	void (*const deinit) (void* const),
	const drawer_t drawer);

// Note: the drawer returns if the mouse should be visible.

#endif
