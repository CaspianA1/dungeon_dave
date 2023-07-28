#include "rendering/entities/billboard.h"
#include "utils/macro_utils.h" // For `ON_FIRST_CALL`
#include "utils/opengl_wrappers.h" // For various OpenGL wrappers
#include "utils/shader.h" // For `init_shader`

/* TODO:
- Fix weird depth clamping errors when billboard intersect with the near plane
- Note: culling cannot be done for billboards for shadow mapping, for the same reason as with sectors
- Render the player body as a shadowed billboard that always has the same center as the camera
- Maybe render billboards as a 3D texture, so that smoother animations can happen (if no blending happens across sections)

Drawing billboards to the shadow cascades (an ideal version):
	- Update billboards on the CPU
	- Sort billboards back-to-front
	- Enable blending
	- Draw them with a special depth shader that gets billboard vertices, and outputs to a 8-bit alpha texture
		(perhaps at half-res, for less memory usage? And perhaps no cascades, since no depth range problems aren't there?)
		(and for that, sort billboards in terms of the light pos (how to do that with no pos?))
	- Then, when drawing entities that use the world shading fragment shader, multiply the shadow value by the alpha value
*/

typedef struct {
	billboard_index_t index;
	GLfloat dist_to_camera_squared;
} BillboardDistanceSortRef;

//////////

// This just updates the billboard animation instances at the moment
void update_billboard_context(const BillboardContext* const billboard_context, const GLfloat curr_time_secs) {
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
			animation_data[animation_instance -> animation_index],
			&billboard_data[animation_instance -> billboard_index].texture_id);
	}
}

////////// This part concerns the sorting of billboard indices from back to front, and rendering

// Using insertion sort because the data is already partially sorted. Returns if the data needed to be sorted.
static void sort_billboard_refs_backwards(BillboardDistanceSortRef* const sort_refs, const billboard_index_t num_billboards) {
	for (billboard_index_t i = 1; i < num_billboards; i++) {
		const BillboardDistanceSortRef sort_ref = sort_refs[i];

		/* `j` may become smaller than 0, so using a type
		that fits `billboard_index_t` that's signed */
		int32_t j = i - 1;

		while (j >= 0 && sort_refs[j].dist_to_camera_squared < sort_ref.dist_to_camera_squared) {
			sort_refs[j + 1] = sort_refs[j];
			j--;
		}

		sort_refs[j + 1] = sort_ref;
	}
}

static void sort_billboards_by_dist_to_camera(BillboardContext* const billboard_context, const vec3 camera_pos) {
	const List* const billboards = &billboard_context -> billboards;

	const Billboard* const billboard_data = billboards -> data;
	BillboardDistanceSortRef* const sort_ref_data = billboard_context -> distance_sort_refs.data;

	const billboard_index_t num_billboards = (billboard_index_t) billboards -> length;

	////////// Reinitializing the sort ref distances to the camera, and sorting the billboards

	for (billboard_index_t i = 0; i < num_billboards; i++) {
		BillboardDistanceSortRef* const sort_ref = sort_ref_data + i;

		sort_ref -> dist_to_camera_squared = glm_vec3_distance2(
			(GLfloat*) camera_pos,
			(GLfloat*) billboard_data[sort_ref -> index].pos
		);
	}

	////////// Moving the billboards into their right positions in their GPU buffer

	sort_billboard_refs_backwards(sort_ref_data, num_billboards);

	Billboard* const billboards_gpu = init_vertex_buffer_memory_mapping(
		billboard_context -> drawable.vertex_buffer, num_billboards * sizeof(Billboard), true
	);

	// TODO: do culling via an AABB tree later
	for (billboard_index_t i = 0; i < num_billboards; i++)
		billboards_gpu[i] = billboard_data[sort_ref_data[i].index];

	deinit_vertex_buffer_memory_mapping();
}

//////////

void draw_billboards_to_shadow_context(const BillboardContext* const billboard_context) {
	const GLuint depth_shader = billboard_context -> shadow_mapping.depth_shader;
	const Drawable* const drawable = &billboard_context -> drawable;

	use_shader(depth_shader);

	ON_FIRST_CALL(
		INIT_UNIFORM_VALUE(alpha_threshold, depth_shader, 1f,
			billboard_context -> shadow_mapping.alpha_threshold);

		// For alpha-tested shadows
		use_texture_in_shader(drawable -> albedo_texture, depth_shader,
			"albedo_sampler", TexSet, TU_BillboardAlbedo);
	);

	WITHOUT_BINARY_RENDER_STATE(GL_CULL_FACE,
		draw_drawable(*drawable, corners_per_quad,
			billboard_context -> billboards.length, NULL, UseVertexSpec
		);
	);
}

// This is just a utility function
void draw_billboards(BillboardContext* const billboard_context, const Camera* const camera) {
	sort_billboards_by_dist_to_camera(billboard_context, camera -> pos);

	draw_drawable(billboard_context -> drawable, corners_per_quad, billboard_context -> billboards.length,
		NULL, UseShaderPipeline | UseVertexSpec
	);
}

////////// Initialization and deinitialization

static void define_vertex_spec(void) {
	define_vertex_spec_index(true, false, 0, 1, sizeof(Billboard), offsetof(Billboard, material_index), MATERIAL_INDEX_TYPENAME);
	define_vertex_spec_index(true, false, 1, 1, sizeof(Billboard), offsetof(Billboard, texture_id), TEXTURE_ID_TYPENAME);
	define_vertex_spec_index(true, true, 2, 1, sizeof(Billboard), offsetof(Billboard, scale), GL_FLOAT);
	define_vertex_spec_index(true, true, 3, 3, sizeof(Billboard), offsetof(Billboard, pos), GL_FLOAT);
}

// TODO: avoid passing in the num animation layouts (it equals the num billboard animations)
BillboardContext init_billboard_context(
	const GLfloat shadow_mapping_alpha_threshold,
	const MaterialPropertiesPerObjectType* const shared_material_properties,

	const texture_id_t num_animation_layouts, const AnimationLayout* const animation_layouts,

	const billboard_index_t num_still_textures, const GLchar* const* const still_texture_paths,
	const billboard_index_t num_billboards, Billboard* const billboards,
	const billboard_index_t num_billboard_animations, Animation* const billboard_animations,
	const billboard_index_t num_billboard_animation_instances, BillboardAnimationInstance* const billboard_animation_instances) {

	const GLsizei texture_size = shared_material_properties -> texture_rescale_size;

	const GLuint albedo_texture_set = init_texture_set(
		true, TexNonRepeating, OPENGL_LEVEL_MAG_FILTER, OPENGL_LEVEL_MIN_FILTER,
		num_still_textures, num_animation_layouts, texture_size, texture_size, still_texture_paths, animation_layouts
	);

	BillboardContext billboard_context = {
		.drawable = init_drawable_with_vertices(
			define_vertex_spec, NULL, GL_DYNAMIC_DRAW, GL_TRIANGLE_STRIP,
			(List) {.data = NULL, .item_size = sizeof(Billboard), .length = num_billboards},

			init_shader("shaders/billboard.vert", NULL, "shaders/common/world_shading.frag", NULL),
			albedo_texture_set, init_normal_map_from_albedo_texture(albedo_texture_set,
				TexSet, &shared_material_properties -> normal_map_config
			)
		),

		.shadow_mapping = {
			.depth_shader = init_shader(
				"shaders/shadow/billboard_depth.vert",
				"shaders/shadow/billboard_depth.geom",
				"shaders/shadow/billboard_depth.frag",
				NULL
			),

			.alpha_threshold = shadow_mapping_alpha_threshold
		},

		.distance_sort_refs = init_list(num_billboards, BillboardDistanceSortRef),
		.billboards = {billboards, sizeof(Billboard), num_billboards, num_billboards},

		.animation_instances = {
			billboard_animation_instances, sizeof(BillboardAnimationInstance),
			num_billboard_animation_instances, num_billboard_animation_instances
		},

		.animations = {billboard_animations, sizeof(Animation), num_billboard_animations, num_billboard_animations}
	};

	////////// Initializing the sort refs

	billboard_context.distance_sort_refs.length = num_billboards;

	for (billboard_index_t i = 0; i < num_billboards; i++)
		((BillboardDistanceSortRef*) billboard_context.distance_sort_refs.data)[i].index = i;

	//////////

	return billboard_context;
}

void deinit_billboard_context(const BillboardContext* const billboard_context) {
	deinit_drawable(billboard_context -> drawable);
	deinit_shader(billboard_context -> shadow_mapping.depth_shader);

	deinit_list(billboard_context -> distance_sort_refs);
	deinit_list(billboard_context -> billboards);
	deinit_list(billboard_context -> animations);
	deinit_list(billboard_context -> animation_instances);
}
