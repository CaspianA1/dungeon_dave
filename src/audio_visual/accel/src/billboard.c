#ifndef BILLBOARD_C
#define BILLBOARD_C

#include "headers/billboard.h"
#include "list.c"
#include "animation.c"
#include "data/shaders.c"

DEF_LIST_INITIALIZER(BillboardAnimationInstance, billboard_animation_instance)

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
static bool is_inside_plane(const vec4 sphere, const vec4 plane) {
	const GLfloat dist_btwn_plane_and_sphere = glm_vec3_dot((GLfloat*) sphere, (GLfloat*) plane) + plane[3];
	return dist_btwn_plane_and_sphere > -sphere[3];
}

static bool billboard_in_view_frustum(const Billboard billboard, const vec4 frustum_planes[6]) {
	const GLfloat half_w = billboard.size[0] * 0.5f, half_h = billboard.size[1] * 0.5f;

	const vec4 sphere = { // For a sphere, first 3 components are position, and last component is radius
		billboard.pos[0], billboard.pos[1], billboard.pos[2],
		sqrtf(half_w * half_w + half_h * half_h)
	};

	return
		is_inside_plane(sphere, frustum_planes[0]) && is_inside_plane(sphere, frustum_planes[1]) &&
		is_inside_plane(sphere, frustum_planes[2]) && is_inside_plane(sphere, frustum_planes[3]) &&
		is_inside_plane(sphere, frustum_planes[4]) && is_inside_plane(sphere, frustum_planes[5]);
}

static void draw_billboards(const BatchDrawContext* const draw_context,
	const Camera* const camera, const buffer_size_t num_visible_billboards) {

	const GLuint shader = draw_context -> shader;
	glUseProgram(shader);

	static GLint right_xz_world_space_id, model_view_projection_id;
	static bool first_call = true;

	if (first_call) {
		INIT_UNIFORM(right_xz_world_space, shader);
		INIT_UNIFORM(model_view_projection, shader);

		use_texture(draw_context -> texture_set, shader, "texture_sampler", TexSet, BILLBOARD_TEXTURE_UNIT);

		first_call = false;
	}

	UPDATE_UNIFORM(right_xz_world_space, 2f, camera -> right[0], camera -> right[2]);
	UPDATE_UNIFORM(model_view_projection, Matrix4fv, 1, GL_FALSE, &camera -> model_view_projection[0][0]);

	//////////

	for (byte i = 0; i < 3; i++) {
		glEnableVertexAttribArray(i);
		glVertexAttribDivisor(i, 1);
	}

	glVertexAttribIPointer(0, 1, BUFFER_SIZE_TYPENAME, sizeof(Billboard), (void*) 0);
	glVertexAttribPointer(1, 2, BB_POS_COMPONENT_TYPENAME, GL_FALSE, sizeof(Billboard), (void*) offsetof(Billboard, size));
	glVertexAttribPointer(2, 3, BB_POS_COMPONENT_TYPENAME, GL_FALSE, sizeof(Billboard), (void*) offsetof(Billboard, pos));

	glDisable(GL_CULL_FACE);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, corners_per_billboard, num_visible_billboards);
	glDisable(GL_BLEND);

	glEnable(GL_CULL_FACE);

	for (byte i = 0; i < 3; i++) {
		glVertexAttribDivisor(i, 0);
		glDisableVertexAttribArray(i);
	}
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

		const buffer_size_t num_visible_in_group = billboard - initial_billboard;
		if (num_visible_in_group != 0) {
			memcpy(gpu_billboard_buffer_ptr + num_visible, initial_billboard, num_visible_in_group * sizeof(Billboard));
			num_visible += num_visible_in_group;
		}
	}

	glUnmapBuffer(GL_ARRAY_BUFFER);
	if (num_visible != 0) draw_billboards(draw_context, camera, num_visible);
}

BatchDrawContext init_billboard_draw_context(const buffer_size_t num_billboards, ...) {
	BatchDrawContext draw_context = {
		.buffers.cpu = init_list(num_billboards, Billboard),
		.shader = init_shader_program(billboard_vertex_shader, billboard_fragment_shader)
	};

	draw_context.buffers.cpu.length = num_billboards;

	va_list args;
	va_start(args, num_billboards);
	for (buffer_size_t i = 0; i < num_billboards; i++)
		((Billboard*) (draw_context.buffers.cpu.data))[i] = va_arg(args, Billboard);
	va_end(args);

	init_batch_draw_context_gpu_buffer(&draw_context, num_billboards, sizeof(Billboard));

	return draw_context;
}

#endif
