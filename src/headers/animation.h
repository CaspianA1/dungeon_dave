#ifndef ANIMATION_H
#define ANIMATION_H

#include "buffer_defs.h"

typedef struct {
	const struct {const buffer_size_t start, end;} texture_id_range;
	const GLfloat secs_for_frame;
} Animation;

typedef struct {
	const GLchar* const spritesheet_path;
	const GLsizei frames_across, frames_down, total_frames;
} AnimationLayout;

void update_animation_information(
	const GLfloat curr_time_secs, const GLfloat cycle_base_time,
	const Animation animation, buffer_size_t* const texture_id);

#endif
