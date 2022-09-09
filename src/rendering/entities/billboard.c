#include "rendering/entities/billboard.h"
#include "utils/texture.h"
#include "utils/shader.h"
#include "utils/opengl_wrappers.h"
#include "data/constants.h"

typedef struct {
	billboard_index_t index;
	GLfloat dist_to_camera;
} BillboardDistanceSortRef;

/* TODO:
- Fix weird depth clamping errors when billboard intersect with the near plane
- Note: culling cannot be done for billboards for shadow mapping, for the same reason as with sectors
- Render the player body as a shadowed billboard that always has the same center as the camera
- Maybe render billboards as a 3D texture, so that smoother animations can happen (if no blending happens across sections)

Drawing billboards to the shadow cascades:
	- Update billboards on the CPU
	- Sort billboards back-to-front
	- Enable blending
	- Draw them with a special depth shader that gets billboard vertices, and outputs to a 8-bit alpha texture
		(perhaps at half-res, for less memory usage? And perhaps no cascades, since no depth range problems aren't there?)
	- Then, when drawing entities that use the world shading fragment shader, multiply the shadow value by the alpha value
*/

//////////

// This just updates the billboard animation instances at the moment
void update_billboards(const BillboardContext* const billboard_context, const GLfloat curr_time_secs) {
	const List* const animation_instances = &billboard_context -> animation_instances;

	BillboardAnimationInstance* const animation_instance_data = animation_instances -> data;
	const Animation* const animation_data = billboard_context -> animations.data;
	Billboard* const billboard_data = billboard_context -> billboards.data;

	/* Billboard animations are constantly looping, so their
	cycle base time is for when this function is first called */
	static GLfloat cycle_base_time;
	ON_FIRST_CALL(cycle_base_time = curr_time_secs;);

	for (billboard_index_t i = 0; i < animation_instances -> length; i++) {
		BillboardAnimationInstance* const animation_instance = animation_instance_data + i;

		update_animation_information(
			curr_time_secs, cycle_base_time,
			animation_data[animation_instance -> animation_id],
			&billboard_data[animation_instance -> billboard_id].texture_id);
	}
}

////////// This part concerns the sorting of billboard indices from back to front, and rendering

static int compare_billboard_sort_refs(const void* const a, const void* const b) {
	const GLfloat
		dist_a = ((BillboardDistanceSortRef*) a) -> dist_to_camera,
		dist_b = ((BillboardDistanceSortRef*) b) -> dist_to_camera;

	if (dist_a < dist_b) return 1;
	else if (dist_a > dist_b) return -1;
	else return 0;
}

static void sort_billboards_by_dist_to_camera(BillboardContext* const billboard_context, const vec3 camera_pos) {
	const List* const billboards = &billboard_context -> billboards;

	const Billboard* const billboard_data = billboards -> data;
	BillboardDistanceSortRef* const sort_ref_data = billboard_context -> distance_sort_refs.data;

	const billboard_index_t num_billboards = (billboard_index_t) billboards -> length;

	for (billboard_index_t i = 0; i < num_billboards; i++)
		sort_ref_data[i] = (BillboardDistanceSortRef) {
			i, glm_vec3_distance((GLfloat*) camera_pos, (GLfloat*) billboard_data[i].pos)
		};

	/* Sorting from back to front by index (the actual billboards are not sorted, since that would require much more copying).
	TODO: use the fact that the billboards will already be partially sorted for better sorting performance (i.e. using insertion sort). */
	qsort(sort_ref_data, num_billboards, sizeof(BillboardDistanceSortRef), compare_billboard_sort_refs);

	////////// Moving the billboards into their right positions in their GPU buffer

	Billboard* const billboards_gpu = init_vertex_buffer_memory_mapping(
		billboard_context -> drawable.vertex_buffer, num_billboards * sizeof(Billboard), true
	);

	// TODO: do culling via an AABB tree later
	for (billboard_index_t i = 0; i < num_billboards; i++)
		billboards_gpu[i] = billboard_data[sort_ref_data[i].index];

	deinit_vertex_buffer_memory_mapping();
}

////////// This part concerns the updating of uniforms

typedef struct {
	const GLuint normal_map_set;
	const CascadedShadowContext* const shadow_context;
	const Skybox* const skybox;
	const GLfloat* const right_xz;
	const AmbientOcclusionMap ao_map;
} UniformUpdaterParams;

static void update_uniforms(const Drawable* const drawable, const void* const param) {
	const UniformUpdaterParams typed_params = *(UniformUpdaterParams*) param;
	static GLint tbn_id;

	ON_FIRST_CALL(
		const GLuint shader = drawable -> shader;

		INIT_UNIFORM(tbn, shader);

		use_texture_in_shader(typed_params.skybox -> diffuse_texture, shader, "environment_map_sampler", TexSkybox, TU_Skybox);
		use_texture_in_shader(drawable -> diffuse_texture, shader, "diffuse_sampler", TexSet, TU_BillboardDiffuse);
		use_texture_in_shader(typed_params.normal_map_set, shader, "normal_map_sampler", TexSet, TU_BillboardNormalMap);
		use_texture_in_shader(typed_params.shadow_context -> depth_layers, shader, "shadow_cascade_sampler", shadow_map_texture_type, TU_CascadedShadowMap);
		use_texture_in_shader(typed_params.ao_map, shader, "ambient_occlusion_sampler", TexVolumetric, TU_AmbientOcclusionMap);
	);

	//////////

	const GLfloat right_xz_x = typed_params.right_xz[0], right_xz_z = typed_params.right_xz[1];

	UPDATE_UNIFORM(tbn, Matrix3fv, 1, GL_FALSE, (GLfloat*) (mat3) {
		{right_xz_x, 0.0f, -right_xz_z},
		{0.0f, -1.0f, 0.0f},
		{right_xz_z, 0.0f, right_xz_x}
	});
}

//////////

// This is just a utility function
void draw_billboards(BillboardContext* const billboard_context,
	const CascadedShadowContext* const shadow_context,
	const Skybox* const skybox, const Camera* const camera,
	const AmbientOcclusionMap ao_map) {

	sort_billboards_by_dist_to_camera(billboard_context, camera -> pos);

	draw_drawable(billboard_context -> drawable, corners_per_quad, billboard_context -> billboards.length,
		&(UniformUpdaterParams) {billboard_context -> normal_map_set, shadow_context, skybox, camera -> right_xz, ao_map},
		UseShaderPipeline | BindVertexSpec
	);
}

////////// Initialization and deinitialization

static void define_vertex_spec(void) {
	define_vertex_spec_index(true, false, 0, 1, sizeof(Billboard), 0, BUFFER_SIZE_TYPENAME);
	define_vertex_spec_index(true, true, 1, 2, sizeof(Billboard), offsetof(Billboard, size), BILLBOARD_VAR_COMPONENT_TYPENAME);
	define_vertex_spec_index(true, true, 2, 3, sizeof(Billboard), offsetof(Billboard, pos), BILLBOARD_VAR_COMPONENT_TYPENAME);
}

BillboardContext init_billboard_context(
	const GLsizei texture_size, const NormalMapConfig* const normal_map_config,

	const billboard_index_t num_still_textures, const GLchar* const* const still_texture_paths,
	const billboard_index_t num_animation_layouts, const AnimationLayout* const animation_layouts,

	const billboard_index_t num_billboards, const Billboard* const billboards,
	const billboard_index_t num_billboard_animations, const Animation* const billboard_animations,
	const billboard_index_t num_billboard_animation_instances, const BillboardAnimationInstance* const billboard_animation_instances) {

	const GLuint diffuse_texture_set = init_texture_set(
		true, TexNonRepeating, OPENGL_SCENE_MAG_FILTER, OPENGL_SCENE_MIN_FILTER,
		num_still_textures, num_animation_layouts, texture_size, texture_size, still_texture_paths, animation_layouts
	);

	BillboardContext billboard_context = {
		.drawable = init_drawable_with_vertices(
			define_vertex_spec, (uniform_updater_t) update_uniforms, GL_DYNAMIC_DRAW, GL_TRIANGLE_STRIP,
			(List) {.data = NULL, .item_size = sizeof(Billboard), .length = num_billboards},

			init_shader(ASSET_PATH("shaders/billboard.vert"), NULL, ASSET_PATH("shaders/billboard.frag"), NULL),
			diffuse_texture_set
		),

		.normal_map_set = init_normal_map_from_diffuse_texture(diffuse_texture_set, TexSet, normal_map_config),

		.distance_sort_refs = init_list(num_billboards, BillboardDistanceSortRef),
		.billboards = init_list(num_billboards, Billboard),
		.animation_instances = init_list(num_billboard_animation_instances, BillboardAnimationInstance),
		.animations = init_list(num_billboard_animations, Animation)
	};

	////////// Initializing client-side lists

	push_array_to_list(&billboard_context.billboards, billboards, num_billboards);
	push_array_to_list(&billboard_context.animation_instances, billboard_animation_instances, num_billboard_animation_instances);
	push_array_to_list(&billboard_context.animations, billboard_animations, num_billboard_animations);

	return billboard_context;
}

void deinit_billboard_context(const BillboardContext* const billboard_context) {
	deinit_drawable(billboard_context -> drawable);
	deinit_texture(billboard_context -> normal_map_set);

	deinit_list(billboard_context -> distance_sort_refs);
	deinit_list(billboard_context -> billboards);
	deinit_list(billboard_context -> animations);
	deinit_list(billboard_context -> animation_instances);
}
