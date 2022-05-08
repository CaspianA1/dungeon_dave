#ifndef BILLBOARD_H
#define BILLBOARD_H

#include "buffer_defs.h"
#include "batch_draw_context.h"
#include "esm.h"
#include "texture.h"
#include "camera.h"
#include "animation.h"
#include "list.h"

typedef struct { // This struct is perfectly aligned
	buffer_size_t texture_id;
	billboard_var_component_t size[2], pos[3];
} Billboard;

typedef struct {
	/* The billboard ID is associated with a BillboardAnimationInstance
	and not an Animation because there's one animation instance per billboard. */
	const struct {const buffer_size_t billboard, animation;} ids;
	GLfloat last_frame_time;
} BillboardAnimationInstance;

// Excluded: is_inside_plane, billboard_in_view_frustum, draw_billboards

void update_billboard_animation_instances(const List* const billboard_animation_instances,
	const List* const billboard_animations, const List* const billboards);

void draw_visible_billboards(const BatchDrawContext* const draw_context,
	const ShadowMapContext* const shadow_map_context, const Camera* const camera);

BatchDrawContext init_billboard_draw_context(const buffer_size_t num_billboards, const Billboard* const billboards);

#endif
