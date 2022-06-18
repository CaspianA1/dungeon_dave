#ifndef BILLBOARD_C
#define BILLBOARD_C

#include "headers/billboard.h"
#include "headers/texture.h"
#include "headers/shader.h"
#include "headers/constants.h"

//////////

// This just updates the billboard animation instances at the moment
void update_billboards(const BillboardContext* const billboard_context) {
	const List* const animation_instances = &billboard_context -> animation_instances;

	BillboardAnimationInstance* const animation_instance_data = animation_instances -> data;
	const Animation* const animation_data = billboard_context -> animations.data;
	Billboard* const billboard_data = billboard_context -> draw_context.buffers.cpu.data;

	/* Billboard animations are constantly looping, so their
	cycle base time is for when this function is first called */
	static Uint32 cycle_base_time;
	ON_FIRST_CALL(cycle_base_time = SDL_GetTicks(););

	for (buffer_size_t i = 0; i < animation_instances -> length; i++) {
		BillboardAnimationInstance* const animation_instance = animation_instance_data + i;

		update_animation_information(cycle_base_time,
			&billboard_data[animation_instance -> ids.billboard].texture_id,
			animation_data[animation_instance -> ids.animation]);
	}
}

// TODO: unify billboard culling with sector culling
static bool billboard_in_view_frustum(const Billboard billboard, const vec4 frustum_planes[6]) {
	// TODO: Later, test against single plane (most accurate method)

	vec3 extents = {billboard.size[0], billboard.size[1], billboard.size[0]}, aabb[2];

	glm_vec3_scale(extents, 0.5f, extents);
	glm_vec3_sub((GLfloat*) billboard.pos, extents, aabb[0]);
	glm_vec3_add((GLfloat*) billboard.pos, extents, aabb[1]);

	return glm_aabb_frustum((vec3*) aabb, (vec4*) frustum_planes);
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

		use_texture(draw_context -> texture_set, shader, "texture_sampler", TexSet, BILLBOARD_TEXTURE_UNIT);
		use_texture(shadow_context -> depth_layers, shader, "shadow_cascade_sampler", TexSet, CASCADED_SHADOW_MAP_TEXTURE_UNIT);
	);

	UPDATE_UNIFORM(right_xz_world_space, 2f, camera -> right_xz[0], camera -> right_xz[1]);
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

void draw_visible_billboards(const BillboardContext* const billboard_context,
	const CascadedShadowContext* const shadow_context, const Camera* const camera) {

	const BatchDrawContext* const draw_context = &billboard_context -> draw_context;

	use_vertex_buffer(draw_context -> buffers.gpu);

	const List cpu_billboards = draw_context -> buffers.cpu;
	const vec4* const frustum_planes = camera -> frustum_planes;
	Billboard* const gpu_billboard_buffer = init_mapping_for_culled_batching(draw_context);

	buffer_size_t num_visible_billboards = 0;
	const Billboard* const out_of_bounds_billboard = ((Billboard*) cpu_billboards.data) + cpu_billboards.length;

	for (const Billboard* billboard = cpu_billboards.data; billboard < out_of_bounds_billboard; billboard++) {
		const Billboard* const initial_billboard = billboard;
		while (billboard < out_of_bounds_billboard && billboard_in_view_frustum(*billboard, frustum_planes))
			billboard++;

		const buffer_size_t num_visible_in_group = (buffer_size_t) (billboard - initial_billboard);
		if (num_visible_in_group != 0) {
			memcpy(gpu_billboard_buffer + num_visible_billboards, initial_billboard, num_visible_in_group * sizeof(Billboard));
			num_visible_billboards += num_visible_in_group;
		}
	}

	deinit_current_mapping_for_culled_batching();
	if (num_visible_billboards != 0) draw_billboards(draw_context, shadow_context, camera, num_visible_billboards);
}

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
