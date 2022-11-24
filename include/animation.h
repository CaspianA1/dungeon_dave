#ifndef ANIMATION_H
#define ANIMATION_H

#include "utils/typedefs.h" // For OpenGL types + other typedefs

typedef struct { // Note: the texture id range is inclusive.
	const struct {const texture_id_t start, end;} texture_id_range;
	const GLfloat secs_for_frame;
} Animation;

typedef struct {
	const GLchar* const spritesheet_path;
	const texture_id_t frames_across, frames_down, total_frames;
} AnimationLayout;

void update_animation_information(
	const GLfloat curr_time_secs, const GLfloat cycle_base_time,
	const Animation animation, texture_id_t* const texture_id);

#endif
