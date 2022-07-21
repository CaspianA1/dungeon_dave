#ifndef BILLBOARD_H
#define BILLBOARD_H

#include "buffer_defs.h"
#include "shadow.h"
#include "camera.h"
#include "animation.h"
#include "list.h"

// TODO: make sure that billboards never intersect, because that would break depth sorting

typedef struct { // TODO: integrate this into Drawable
	const GLuint vertex_buffer, vertex_spec, diffuse_texture_set, normal_map_set, shader;
	List distance_sort_refs, billboards, animations, animation_instances;
} BillboardContext;

typedef struct { // This struct is perfectly aligned
	buffer_size_t texture_id;
	billboard_var_component_t size[2];
	billboard_var_component_t pos[3];
} Billboard;

typedef struct {
	/* The billboard ID is associated with a BillboardAnimationInstance
	and not an Animation because there's one animation instance per billboard. */
	const struct {const billboard_index_t billboard, animation;} ids;
} BillboardAnimationInstance;

////////// Excluded: internal_draw_billboards, compare_billboard_sort_refs, sort_billboard_indices_by_dist_to_camera

void update_billboards(const BillboardContext* const billboard_context, const GLfloat curr_time_secs);

void draw_billboards(BillboardContext* const billboard_context,
	const CascadedShadowContext* const shadow_context, const Camera* const camera);

BillboardContext init_billboard_context(const GLuint diffuse_texture_set,
	const billboard_index_t num_billboards, const Billboard* const billboards,
	const billboard_index_t num_billboard_animations, const Animation* const billboard_animations,
	const billboard_index_t num_billboard_animation_instances, const BillboardAnimationInstance* const billboard_animation_instances);

void deinit_billboard_context(const BillboardContext* const billboard_context);

#endif
