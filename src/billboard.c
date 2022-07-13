#ifndef BILLBOARD_C
#define BILLBOARD_C

#include "headers/billboard.h"
#include "headers/texture.h"
#include "headers/shader.h"
#include "headers/constants.h"

/* TODO:
- Fix weird depth clamping errors when billboard intersect with the near plane

- Note: culling cannot be done for billboards for shadow mapping, for the same reason as with sectors

Drawing billboards to the shadow cascades:
	- Update billboards on the CPU
	- Keep a transform feedback buffer, and use transform feedback to write 4 corners for each into that
	- Draw that transform feedback buffer in an instanced manner, so that 2 triangle strips exist per billboard
	- Handle alpha in some way (I am not sure how to do that yet)

	- Cull billboards into the billboard GPU buffer
	- Draw them as normally, with glDrawArraysInstanced

Other idea, with a tweaked shader (instanced + triangle strip + alpha to coverage):
	- Vertex shader generates billboard vertices and a texture id for the fragment shader
	- Geometry shader does nothing different
	- Fragment shader discards fragments based on the billboard alpha value (done automatically, even if no color buffer exists?)
*/

// This just updates the billboard animation instances at the moment
void update_billboards(const BillboardContext* const billboard_context, const GLfloat curr_time_secs) {
	const List* const animation_instances = &billboard_context -> animation_instances;

	BillboardAnimationInstance* const animation_instance_data = animation_instances -> data;
	const Animation* const animation_data = billboard_context -> animations.data;
	Billboard* const billboard_data = billboard_context -> draw_context.buffers.cpu.data;

	/* Billboard animations are constantly looping, so their
	cycle base time is for when this function is first called */
	static GLfloat cycle_base_time;
	ON_FIRST_CALL(cycle_base_time = curr_time_secs;);

	for (buffer_size_t i = 0; i < animation_instances -> length; i++) {
		BillboardAnimationInstance* const animation_instance = animation_instance_data + i;

		update_animation_information(
			curr_time_secs, cycle_base_time,
			animation_data[animation_instance -> ids.animation],
			&billboard_data[animation_instance -> ids.billboard].texture_id);
	}
}

static void draw_billboards(const BatchDrawContext* const draw_context,
	const CascadedShadowContext* const shadow_context, const Camera* const camera,
	const buffer_size_t num_visible_billboards) {

	const GLuint shader = draw_context -> shader;
	static GLint right_xz_world_space_id, view_projection_id, camera_view_id, light_view_projection_matrices_id;

	use_shader(shader);

	ON_FIRST_CALL(
		INIT_UNIFORM(right_xz_world_space, shader);
		INIT_UNIFORM(view_projection, shader);
		INIT_UNIFORM(camera_view, shader);
		INIT_UNIFORM(light_view_projection_matrices, shader);

		INIT_UNIFORM_VALUE(ambient, shader, 1f, constants.lighting.ambient);

		const List* const split_dists = &shadow_context -> split_dists;
		INIT_UNIFORM_VALUE(cascade_split_distances, shader, 1fv, (GLsizei) split_dists -> length, split_dists -> data);

		use_texture(draw_context -> texture_set, shader, "texture_sampler", TexSet, TU_Billboard);
		use_texture(shadow_context -> depth_layers, shader, "shadow_cascade_sampler", TexSet, TU_CascadedShadowMap);
	);

	const GLfloat* const right_xz = camera -> right_xz;
	UPDATE_UNIFORM(right_xz_world_space, 2f, right_xz[0], right_xz[1]);
	UPDATE_UNIFORM(view_projection, Matrix4fv, 1, GL_FALSE, &camera -> view_projection[0][0]);

	////////// This little part concerns CSM

	UPDATE_UNIFORM(camera_view, Matrix4fv, 1, GL_FALSE, &camera -> view[0][0]);

	const List* const light_view_projection_matrices = &shadow_context -> light_view_projection_matrices;

	UPDATE_UNIFORM(light_view_projection_matrices, Matrix4fv,
		(GLsizei) light_view_projection_matrices -> length, GL_FALSE,
		light_view_projection_matrices -> data);

	//////////

	use_vertex_spec(draw_context -> vertex_spec);

	WITHOUT_BINARY_RENDER_STATE(GL_CULL_FACE,
		/* Not using blending or alpha testing because they're
		not order independent and they have some funky artifacts */
		WITH_BINARY_RENDER_STATE(GL_SAMPLE_ALPHA_TO_COVERAGE,
			glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, corners_per_quad, (GLsizei) num_visible_billboards);
		);
	);
}

////////// These functions are for frustum culling

static void make_aabb(const byte* const typeless_billboard, vec3 aabb[2]) {
	const Billboard* const billboard = (Billboard*) typeless_billboard;
	const GLfloat *const size = billboard -> size, *const pos = billboard -> pos;

	vec3 extents = {size[0], size[1], size[0]};

	glm_vec3_scale(extents, 0.5f, extents);
	glm_vec3_sub((GLfloat*) pos, extents, aabb[0]);
	glm_vec3_add((GLfloat*) pos, extents, aabb[1]);
}

static buffer_size_t get_renderable_index_from_cullable(const byte* const typeless_billboard, const byte* const typeless_first_billboard) {
	return (buffer_size_t) ((Billboard*) typeless_billboard - (Billboard*) typeless_first_billboard);
}

static buffer_size_t get_num_renderable_from_cullable(const byte* const typeless_billboard) {
	(void) typeless_billboard;
	return 1;
}

//////////

// This is just a utility function
void draw_visible_billboards(const BillboardContext* const billboard_context,
	const CascadedShadowContext* const shadow_context, const Camera* const camera) {

	const BatchDrawContext* const draw_context = &billboard_context -> draw_context;

	const buffer_size_t num_visible_billboards = cull_from_frustum_into_gpu_buffer(
		draw_context, draw_context -> buffers.cpu,
		camera -> frustum_planes, make_aabb,
		get_renderable_index_from_cullable,
		get_num_renderable_from_cullable
	);

	if (num_visible_billboards != 0)
		draw_billboards(draw_context, shadow_context, camera, num_visible_billboards);
}

//////////

BillboardContext init_billboard_context(const GLuint diffuse_texture_set,
	const buffer_size_t num_billboards, const Billboard* const billboards,
	const buffer_size_t num_billboard_animations, const Animation* const billboard_animations,
	const buffer_size_t num_billboard_animation_instances, const BillboardAnimationInstance* const billboard_animation_instances) {

	BillboardContext billboard_context = {
		.draw_context = {
			.buffers.cpu = init_list(num_billboards, Billboard),
			.vertex_spec = init_vertex_spec(),
			.texture_set = diffuse_texture_set,
			.shader = init_shader(ASSET_PATH("shaders/billboard.vert"), NULL, ASSET_PATH("shaders/billboard.frag"))
		},

		.animations = init_list(num_billboard_animations, Animation),
		.animation_instances = init_list(num_billboard_animation_instances, BillboardAnimationInstance)
	};

	push_array_to_list(&billboard_context.draw_context.buffers.cpu, billboards, num_billboards);
	init_batch_draw_context_gpu_buffer(&billboard_context.draw_context, num_billboards, sizeof(Billboard));

	use_vertex_spec(billboard_context.draw_context.vertex_spec);
	define_vertex_spec_index(true, false, 0, 1, sizeof(Billboard), 0, BUFFER_SIZE_TYPENAME);
	define_vertex_spec_index(true, true, 1, 2, sizeof(Billboard), offsetof(Billboard, size), BILLBOARD_VAR_COMPONENT_TYPENAME);
	define_vertex_spec_index(true, true, 2, 3, sizeof(Billboard), offsetof(Billboard, pos), BILLBOARD_VAR_COMPONENT_TYPENAME);

	//////////

	push_array_to_list(&billboard_context.animations, billboard_animations, num_billboard_animations);
	push_array_to_list(&billboard_context.animation_instances, billboard_animation_instances, num_billboard_animation_instances);

	return billboard_context;
}

void deinit_billboard_context(const BillboardContext* const billboard_context) {
	deinit_list(billboard_context -> animation_instances);
	deinit_list(billboard_context -> animations);
	deinit_batch_draw_context(&billboard_context -> draw_context);
}

#endif
