#ifndef BILLBOARD_H
#define BILLBOARD_H

#include "rendering/drawable.h" // For `Drawable`
#include "utils/typedefs.h" // For OpenGL types + other typedefs
#include "utils/list.h" // For `List`
#include "cglm/cglm.h" // For `vec2` and `vec3`
#include "camera.h" // For `Camera`
#include "level_config.h" // For `MaterialPropertiesPerObjectType`
#include "animation.h" // For `Animation`

/* TODO:
- Make sure that billboards never intersect, because that would break depth sorting
- Instead of providing a size for billboards, only provide a scale factor for them, and then
	determine their size by the aspect ratio of a frame in their spritesheet (just provide a `BillboardConfig` struct
	for each billboard)
*/

typedef struct {
	const Drawable drawable;

	const struct {
		const GLuint depth_shader;
		const GLfloat alpha_threshold;
	} shadow_mapping;

	List distance_sort_refs, billboards, animations, animation_instances;
} BillboardContext;

typedef struct {
	material_index_t material_index;
	texture_id_t texture_id;
	vec2 size;
	vec3 pos;
} Billboard;

typedef struct {
	/* The billboard index is associated with a BillboardAnimationInstance
	and not an Animation because there's one animation instance per animated billboard. */
	const billboard_index_t billboard_index, animation_index;
} BillboardAnimationInstance;

////////// Excluded: compare_billboard_sort_refs, sort_billboard_refs_backwards, sort_billboards_by_dist_to_camera, define_vertex_spec

void update_billboard_context(const BillboardContext* const billboard_context, const GLfloat curr_time_secs);

void draw_billboards_to_shadow_context(const BillboardContext* const billboard_context);
void draw_billboards(BillboardContext* const billboard_context, const Camera* const camera);

BillboardContext init_billboard_context(
	const GLfloat shadow_mapping_alpha_threshold,
	const MaterialPropertiesPerObjectType* const shared_material_properties,

	const texture_id_t num_animation_layouts, const AnimationLayout* const animation_layouts,

	const billboard_index_t num_still_textures, const GLchar* const* const still_texture_paths,
	const billboard_index_t num_billboards, const Billboard* const billboards,
	const billboard_index_t num_billboard_animations, const Animation* const billboard_animations,
	const billboard_index_t num_billboard_animation_instances, const BillboardAnimationInstance* const billboard_animation_instances);

void deinit_billboard_context(const BillboardContext* const billboard_context);

#endif
