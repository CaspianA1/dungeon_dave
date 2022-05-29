#ifndef BILLBOARD_C
#define BILLBOARD_C

#include "headers/billboard.h"
#include "headers/shader.h"
#include "headers/constants.h"

typedef struct {
	const vec3 center;
	const GLfloat radius;
} Sphere;

//////////

void update_billboard_animation_instances(const List* const billboard_animation_instances,
	const List* const billboard_animations, const List* const billboards) {

	BillboardAnimationInstance* const billboard_animation_instance_data = billboard_animation_instances -> data;
	const Animation* const animation_data = billboard_animations -> data;
	Billboard* const billboard_data = billboards -> data;

	/* Billboard animations are constantly looping, so their
	cycle base time is for when this function is first called */
	static Uint32 cycle_base_time;
	ON_FIRST_CALL(cycle_base_time = SDL_GetTicks(););

	for (buffer_size_t i = 0; i < billboard_animation_instances -> length; i++) {
		BillboardAnimationInstance* const billboard_animation_instance = billboard_animation_instance_data + i;
		update_animation_information(cycle_base_time,
			&billboard_data[billboard_animation_instance -> ids.billboard].texture_id,
			animation_data[billboard_animation_instance -> ids.animation]);
	}
}

// https://stackoverflow.com/questions/25572337/frustum-and-sphere-intersection
static bool is_inside_plane(const Sphere sphere, const vec4 plane) {
	const GLfloat dist_btwn_plane_and_sphere = glm_vec3_dot((GLfloat*) sphere.center, (GLfloat*) plane) + plane[3];
	return dist_btwn_plane_and_sphere > -sphere.radius;
}

static bool billboard_in_view_frustum(const Billboard billboard, const vec4 frustum_planes[6]) {
	const Sphere sphere = { // For a sphere, the first 3 components are position, and the last component is radius
		.center = {billboard.pos[0], billboard.pos[1], billboard.pos[2]},
		.radius = glm_vec2_norm((vec2) {billboard.size[0], billboard.size[1]}) * 0.5f
	};

	for (byte i = 0; i < 6; i++) {
		if (!is_inside_plane(sphere, frustum_planes[i])) return false;
	}

	return true;
}

static void draw_billboards(const BatchDrawContext* const draw_context,
	const ShadowMapContext* const shadow_map_context, const Camera* const camera,
	const buffer_size_t num_visible_billboards) {

	const GLuint shader = draw_context -> shader;
	static GLint right_xz_world_space_id, model_view_projection_id;

	use_shader(shader);

	ON_FIRST_CALL(
		INIT_UNIFORM(right_xz_world_space, shader);
		INIT_UNIFORM(model_view_projection, shader);

		INIT_UNIFORM_VALUE(ambient, shader, 1f, constants.lighting.ambient);
		INIT_UNIFORM_VALUE(pcf_radius, shader, 1i, constants.lighting.pcf_radius);
		INIT_UNIFORM_VALUE(esm_constant, shader, 1f, constants.lighting.esm_constant);

		INIT_UNIFORM_VALUE(biased_light_model_view_projection, shader, Matrix4fv, 1,
			GL_FALSE, &shadow_map_context -> light.biased_model_view_projection[0][0]);

		use_texture(shadow_map_context -> buffers.depth_texture, shader, "shadow_map_sampler", TexPlain, SHADOW_MAP_TEXTURE_UNIT);
		use_texture(draw_context -> texture_set, shader, "texture_sampler", TexSet, BILLBOARD_TEXTURE_UNIT);
	);

	UPDATE_UNIFORM(right_xz_world_space, 2f, camera -> right_xz[0], camera -> right_xz[1]);
	UPDATE_UNIFORM(model_view_projection, Matrix4fv, 1, GL_FALSE, &camera -> model_view_projection[0][0]);

	use_vertex_spec(draw_context -> vertex_spec);

	WITHOUT_BINARY_RENDER_STATE(GL_CULL_FACE,
		WITH_BINARY_RENDER_STATE(GL_BLEND,
			WITH_BINARY_RENDER_STATE(GL_SAMPLE_ALPHA_TO_COVERAGE,
				glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
				glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, corners_per_quad, (GLsizei) num_visible_billboards);
			);
		);
	);
}

void draw_visible_billboards(const BatchDrawContext* const draw_context,
	const ShadowMapContext* const shadow_map_context, const Camera* const camera) {

	use_vertex_buffer(draw_context -> buffers.gpu);

	const List cpu_billboards = draw_context -> buffers.cpu;
	const vec4* const frustum_planes = camera -> frustum_planes;
	Billboard* const gpu_billboard_buffer_ptr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

	buffer_size_t num_visible = 0;
	const Billboard* const out_of_bounds_billboard = ((Billboard*) cpu_billboards.data) + cpu_billboards.length;

	for (const Billboard* billboard = cpu_billboards.data; billboard < out_of_bounds_billboard; billboard++) {

		const Billboard* const initial_billboard = billboard;
		while (billboard < out_of_bounds_billboard && billboard_in_view_frustum(*billboard, frustum_planes))
			billboard++;

		const buffer_size_t num_visible_in_group = (buffer_size_t) (billboard - initial_billboard);
		if (num_visible_in_group != 0) {
			memcpy(gpu_billboard_buffer_ptr + num_visible, initial_billboard, num_visible_in_group * sizeof(Billboard));
			num_visible += num_visible_in_group;
		}
	}

	glUnmapBuffer(GL_ARRAY_BUFFER);
	if (num_visible != 0) draw_billboards(draw_context, shadow_map_context, camera, num_visible);
}

BatchDrawContext init_billboard_draw_context(const buffer_size_t num_billboards, const Billboard* const billboards) {
	BatchDrawContext draw_context = {
		.buffers.cpu = init_list(num_billboards, Billboard),
		.vertex_spec = init_vertex_spec(),
		.shader = init_shader("assets/shaders/billboard.vert", NULL, "assets/shaders/billboard.frag")
	};

	push_array_to_list(&draw_context.buffers.cpu, billboards, num_billboards);

	init_batch_draw_context_gpu_buffer(&draw_context, num_billboards, sizeof(Billboard));

	use_vertex_spec(draw_context.vertex_spec);
	define_vertex_spec_index(true, false, 0, 1, sizeof(Billboard), 0, BUFFER_SIZE_TYPENAME);
	define_vertex_spec_index(true, true, 1, 2, sizeof(Billboard), offsetof(Billboard, size), BILLBOARD_VAR_COMPONENT_TYPENAME);
	define_vertex_spec_index(true, true, 2, 3, sizeof(Billboard), offsetof(Billboard, pos), BILLBOARD_VAR_COMPONENT_TYPENAME);

	return draw_context;
}

#endif
