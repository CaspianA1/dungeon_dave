#ifndef BILLBOARD_H
#define BILLBOARD_H

#include "rendering/drawable.h"
#include "utils/buffer_defs.h"
#include "utils/list.h"
#include "rendering/shadow.h"
#include "rendering/entities/skybox.h"
#include "camera.h"
#include "normal_map_generation.h"
#include "animation.h"

// TODO: make sure that billboards never intersect, because that would break depth sorting

typedef struct {
	const Drawable drawable;
	const GLuint normal_map_set;
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

////////// Excluded: compare_billboard_sort_refs, sort_billboard_indices_by_dist_to_camera, update_uniforms, define_vertex_spec

void update_billboards(const BillboardContext* const billboard_context, const GLfloat curr_time_secs);

void draw_billboards(BillboardContext* const billboard_context,
	const CascadedShadowContext* const shadow_context,
	const Skybox* const skybox, const Camera* const camera);

BillboardContext init_billboard_context(
	const GLsizei texture_size, const NormalMapConfig* const normal_map_config,

	const billboard_index_t num_still_textures, const GLchar* const* const still_texture_paths,
	const billboard_index_t num_animation_layouts, const AnimationLayout* const animation_layouts,

	const billboard_index_t num_billboards, const Billboard* const billboards,
	const billboard_index_t num_billboard_animations, const Animation* const billboard_animations,
	const billboard_index_t num_billboard_animation_instances, const BillboardAnimationInstance* const billboard_animation_instances);

void deinit_billboard_context(const BillboardContext* const billboard_context);

#endif
