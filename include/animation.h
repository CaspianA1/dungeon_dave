#ifndef ANIMATION_H
#define ANIMATION_H

#include "glad/glad.h" // For OpenGL defs
#include "utils/typedefs.h" // For various typedefs
#include <stdbool.h> // For `bool`

typedef struct { // TODO: add back the const qualifiers, if possible
	const GLchar* spritesheet_path;
	texture_id_t frames_across, frames_down, total_frames;
	GLfloat secs_for_frame;
} AnimationLayout;

typedef struct { // Note: the texture id range is inclusive.
	material_index_t material_index;
	struct {texture_id_t start, end;} texture_id_range;
	GLfloat secs_for_frame;
} Animation;

// This returns if the animation just finished a cycle.
bool update_animation_information(
	const GLfloat curr_time_secs, const Animation animation,
	GLfloat* const cycle_start_time, texture_id_t* const texture_id);

#endif
