#ifndef BILLBOARD_H
#define BILLBOARD_H

#include "buffer_defs.h"
#include "batch_draw_context.h"
#include "camera.h"

typedef struct { // This struct is perfectly aligned
	buffer_size_t texture_id;
	bb_pos_component_t size[2], pos[3];
} Billboard;

typedef struct {
	const struct {const buffer_size_t start, end;} texture_id_range;
	const GLfloat secs_per_frame;
} Animation;

typedef struct {
	/* billboard_id assoc with AnimationInstance and not
	Animation b/c one animation instance per billboard */
	const struct {const buffer_size_t billboard, animation;} ids;
	GLfloat last_frame_time;
} AnimationInstance;

// Excluded: is_inside_plane, billboard_in_view_frustum, draw_billboards

LIST_INITIALIZER_SIGNATURE(Animation, animation);
LIST_INITIALIZER_SIGNATURE(AnimationInstance, animation_instance);

void update_animation_instances(const List* const animation_instances,
	const List* const animations, const List* const billboards);

void draw_visible_billboards(const BatchDrawContext* const draw_context, const Camera* const camera);
BatchDrawContext init_billboard_draw_context(const buffer_size_t num_billboards, ...);

#endif
