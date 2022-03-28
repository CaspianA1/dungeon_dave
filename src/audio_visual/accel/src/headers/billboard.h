#ifndef BILLBOARD_H
#define BILLBOARD_H

#include "buffer_defs.h"
#include "batch_draw_context.h"
#include "texture.h"
#include "camera.h"
#include "animation.h"

typedef struct { // This struct is perfectly aligned
	buffer_size_t texture_id;
	billboard_var_component_t size[2], pos[3];
} Billboard;

typedef struct {
	/* billboard_id assoc with BillboardAnimationInstance and not
	Animation b/c one animation instance per billboard */
	const struct {const buffer_size_t billboard, animation;} ids;
	GLfloat last_frame_time;
} BillboardAnimationInstance;

// Excluded: is_inside_plane, billboard_in_view_frustum, draw_billboards

LIST_INITIALIZER_SIGNATURE(BillboardAnimationInstance, billboard_animation_instance);

void update_billboard_animation_instances(const List* const billboard_animation_instances,
	const List* const billboard_animations, const List* const billboards);

void draw_visible_billboards(const BatchDrawContext* const draw_context, const Camera* const camera);
BatchDrawContext init_billboard_draw_context(const buffer_size_t num_billboards, ...);

#endif
