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

void update_animation_information(const Uint32 cycle_base_time, buffer_size_t* const texture_id, const Animation animation);

#endif
