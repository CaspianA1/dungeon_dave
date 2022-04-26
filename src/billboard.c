#ifndef BILLBOARD_C
#define BILLBOARD_C

#include "headers/billboard.h"
#include "headers/shaders.h"

void update_billboard_animation_instances(const List* const billboard_animation_instances,
	const List* const billboard_animations, const List* const billboards) {

	const GLfloat curr_time = SDL_GetTicks() / 1000.0f;

	BillboardAnimationInstance* const billboard_animation_instance_data = billboard_animation_instances -> data;
	const Animation* const animation_data = billboard_animations -> data;
	Billboard* const billboard_data = billboards -> data;

	for (buffer_size_t i = 0; i < billboard_animation_instances -> length; i++) {
		BillboardAnimationInstance* const billboard_animation_instance = billboard_animation_instance_data + i;
		update_animation_information(&billboard_animation_instance -> last_frame_time,
			&billboard_data[billboard_animation_instance -> ids.billboard].texture_id,
			animation_data[billboard_animation_instance -> ids.animation], curr_time);
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
	const Camera* const camera, const buffer_size_t num_visible_billboards) {

	const GLuint shader = draw_context -> shader;
	static GLint right_xz_world_space_id, model_view_projection_id;

	use_shader(shader);

	ON_FIRST_CALL(
		INIT_UNIFORM(right_xz_world_space, shader);
		INIT_UNIFORM(model_view_projection, shader);
		use_texture(draw_context -> texture_set, shader, "texture_sampler", TexSet, BILLBOARD_TEXTURE_UNIT);
	);

	UPDATE_UNIFORM(right_xz_world_space, 2f, camera -> right_xz[0], camera -> right_xz[1]);
	UPDATE_UNIFORM(model_view_projection, Matrix4fv, 1, GL_FALSE, &camera -> model_view_projection[0][0]);

	WITH_INTEGER_VERTEX_ATTRIBUTE(true, 0, 1, BUFFER_SIZE_TYPENAME, sizeof(Billboard), 0,
		WITH_VERTEX_ATTRIBUTE(true, 1, 2, BILLBOARD_VAR_COMPONENT_TYPENAME, sizeof(Billboard), offsetof(Billboard, size),
			WITH_VERTEX_ATTRIBUTE(true, 2, 3, BILLBOARD_VAR_COMPONENT_TYPENAME, sizeof(Billboard), offsetof(Billboard, pos),

				WITHOUT_BINARY_RENDER_STATE(GL_CULL_FACE,
					WITH_BINARY_RENDER_STATE(GL_BLEND,
						WITH_BINARY_RENDER_STATE(GL_SAMPLE_ALPHA_TO_COVERAGE,

							glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
							glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, corners_per_quad, (GLsizei) num_visible_billboards);
						);
					);
				);
			);
		);
	);
}

void draw_visible_billboards(const BatchDrawContext* const draw_context, const Camera* const camera) {
	glBindBuffer(GL_ARRAY_BUFFER, draw_context -> buffers.gpu);

	const List cpu_billboards = draw_context -> buffers.cpu;
	const vec4* const frustum_planes = camera -> frustum_planes;
	Billboard* const gpu_billboard_buffer_ptr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

	buffer_size_t num_visible = 0;
	const Billboard* const out_of_bounds_billboard = ((Billboard*) cpu_billboards.data) + cpu_billboards.length;

	for (const Billboard* billboard = (Billboard*) cpu_billboards.data; billboard < out_of_bounds_billboard; billboard++) {

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
	if (num_visible != 0) draw_billboards(draw_context, camera, num_visible);
}

BatchDrawContext init_billboard_draw_context(const buffer_size_t num_billboards, const Billboard* const billboards) {
	BatchDrawContext draw_context = {
		.buffers.cpu = init_list(num_billboards, Billboard),
		.shader = init_shader(billboard_vertex_shader, billboard_fragment_shader)
	};

	push_array_to_list(&draw_context.buffers.cpu, billboards, num_billboards);
	init_batch_draw_context_gpu_buffer(&draw_context, num_billboards, sizeof(Billboard));

	return draw_context;
}

#endif
