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

/* How billboard data is layed out:
- In the input JSON file, the level writer provides lists of:
	- Still billboard texture paths
	- Animated billboard texture specs
	- Unanimated billboards, with indices into the still texture paths
	- Animated billboards, with indices into the animated texture specs

Note the absolute texture ids index into one large shared texture fro all billboard textures, animated and unanimated.

In the actual code, list of these are constructed:
	- AnimationLayout: descriptions of each animated billboard spritesheet, along with timing info.
		Used for making the billboard texture set, and constructing the Animation list.

	- Animation: the animated billboard texture specs, with absolute texture ids, and timing info.
	- BillboardAnimationInstance: pairs of billboards and animations.
	- Billboard: the unanimated and animated billboards merged together, but with proper material indices, and absolute texture ids.

In the actual code, this happens:
	For each animation instance:
		- Get the paired billboard and animation
		- Update the frame cycling for the given billboard
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
	GLfloat scale;
	vec3 pos;
} Billboard;

typedef struct {
	/* The billboard index is associated with a BillboardAnimationInstance
	and not an Animation because there's one animation instance per animated billboard. */
	billboard_index_t billboard_index, animation_index;
} BillboardAnimationInstance;

////////// Excluded: compare_billboard_sort_refs, sort_billboard_refs_backwards, sort_billboards_by_dist_to_camera, define_vertex_spec

void update_billboard_context(const BillboardContext* const billboard_context, const GLfloat curr_time_secs);

void draw_billboards_to_shadow_context(const BillboardContext* const billboard_context);
void draw_billboards(BillboardContext* const billboard_context, const Camera* const camera);

// Note: this takes ownership over the billboards, billboard animations, and billboard animation instances.
BillboardContext init_billboard_context(
	const GLfloat shadow_mapping_alpha_threshold,
	const MaterialPropertiesPerObjectType* const shared_material_properties,

	const texture_id_t num_animation_layouts, const AnimationLayout* const animation_layouts,

	const billboard_index_t num_still_textures, const GLchar* const* const still_texture_paths,
	const billboard_index_t num_billboards, Billboard* const billboards,
	const billboard_index_t num_billboard_animations, Animation* const billboard_animations,
	const billboard_index_t num_billboard_animation_instances, BillboardAnimationInstance* const billboard_animation_instances);

void deinit_billboard_context(const BillboardContext* const billboard_context);

#endif
