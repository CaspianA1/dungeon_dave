#ifndef BILLBOARD_H
#define BILLBOARD_H

#include "buffer_defs.h"
#include "batch_draw_context.h"
#include "csm.h"
#include "camera.h"
#include "animation.h"
#include "list.h"

typedef struct {
	BatchDrawContext draw_context; // TODO: integrate the Drawable code into this later
	List animations, animation_instances;
} BillboardContext;

typedef struct { // This struct is perfectly aligned
	buffer_size_t texture_id;
	billboard_var_component_t size[2], pos[3];
} Billboard;

typedef struct {
	/* The billboard ID is associated with a BillboardAnimationInstance
	and not an Animation because there's one animation instance per billboard. */
	const struct {const buffer_size_t billboard, animation;} ids;
} BillboardAnimationInstance;

////////// Excluded: draw_billboards, make_aabb, get_renderable_index_from_cullable, get_num_renderable_from_cullable

void update_billboards(const BillboardContext* const billboard_context);

void draw_visible_billboards(const BillboardContext* const billboard_context,
	const CascadedShadowContext* const shadow_context, const Camera* const camera);

BillboardContext init_billboard_context(const GLuint diffuse_texture_set,
	const buffer_size_t num_billboards, const Billboard* const billboards,
	const buffer_size_t num_billboard_animations, const Animation* const billboard_animations,
	const buffer_size_t num_billboard_animation_instances, const BillboardAnimationInstance* const billboard_animation_instances);

void deinit_billboard_context(const BillboardContext* const billboard_context);

#endif
