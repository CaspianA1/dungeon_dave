#ifndef ANIMATION_H
#define ANIMATION_H

#include "utils/typedefs.h" // For OpenGL types + other typedefs

typedef struct { // TODO: add back the const qualifiers, if possible
	const GLchar* spritesheet_path;
	texture_id_t frames_across, frames_down, total_frames;
	GLfloat secs_for_frame;
} AnimationLayout;

typedef struct { // Note: the texture id range is inclusive.
	struct {texture_id_t start, end;} texture_id_range;
	GLfloat secs_for_frame;
} Animation;

void update_animation_information(
	const GLfloat curr_time_secs, const GLfloat cycle_base_time,
	const Animation animation, texture_id_t* const texture_id);

#endif
