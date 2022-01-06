#ifndef BILLBOARD_C
#define BILLBOARD_C

#include "headers/billboard.h"
#include "list.c"
#include "data/shaders.c"

// https://stackoverflow.com/questions/25572337/frustum-and-sphere-intersection
static byte is_inside_plane(vec4 sphere, vec4 plane) {
	const float dist_btwn_plane_and_sphere = glm_vec3_dot(sphere, plane) + plane[3];
	return dist_btwn_plane_and_sphere > -sphere[3];
}

static byte billboard_in_view_frustum(const Billboard billboard, vec4 frustum_planes[6]) {
	const float half_w = billboard.size[0] * 0.5f, half_h = billboard.size[1] * 0.5f;	

	vec4 sphere = { // For a sphere, first 3 components are position, and last component is radius
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

	static byte first_call = 1;
	static GLint right_id, view_projection_id;

	if (first_call) {
		right_id = glGetUniformLocation(shader, "right_xz_world_space");
		view_projection_id = glGetUniformLocation(shader, "view_projection");
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		first_call = 0;
	}

	glUseProgram(shader);
	glUniform2f(right_id, camera -> right_xz[0], camera -> right_xz[1]);
	glUniformMatrix4fv(view_projection_id, 1, GL_FALSE, &camera -> view_projection[0][0]);

	//////////

	glBindBuffer(GL_ARRAY_BUFFER, draw_context -> buffers.gpu);

	for (byte i = 0; i < 3; i++) {
		glEnableVertexAttribArray(i);
		glVertexAttribDivisor(i, 1);
	}

	glVertexAttribIPointer(0, 1, BB_TEXTURE_ID_TYPENAME, sizeof(Billboard), (void*) 0);
	glVertexAttribPointer(1, 2, BB_POS_COMPONENT_TYPENAME, GL_FALSE, sizeof(Billboard), (void*) offsetof(Billboard, size));
	glVertexAttribPointer(2, 3, BB_POS_COMPONENT_TYPENAME, GL_FALSE, sizeof(Billboard), (void*) offsetof(Billboard, pos));

	use_texture(draw_context -> texture_set, shader, TexSet);

	glEnable(GL_BLEND);
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, num_visible_billboards); // Each billboard has 4 corners
	glDisable(GL_BLEND);

	for (byte i = 0; i < 3; i++) {
		glVertexAttribDivisor(i, 0);
		glDisableVertexAttribArray(i);
	}
}

void draw_visible_billboards(const BatchDrawContext* const draw_context, const Camera* const camera) {
	static vec4 frustum_planes[6]; // TODO: share computed frustum planes between sector and billboard
	glm_frustum_planes((vec4*) camera -> view_projection, frustum_planes);

	const List cpu_billboards = draw_context -> buffers.cpu;
	Billboard* const gpu_billboard_buffer_ptr = draw_context -> buffers.ptr_gpu;

	buffer_size_t num_visible = 0;
	const Billboard* const out_of_bounds_billboard = ((Billboard*) cpu_billboards.data) + cpu_billboards.length;

	for (const Billboard* billboard = (Billboard*) cpu_billboards.data; billboard < out_of_bounds_billboard; billboard++) {

		buffer_size_t num_visible_in_group = 0;
		const Billboard* const initial_billboard = billboard;

		while (billboard < out_of_bounds_billboard && billboard_in_view_frustum(*billboard, frustum_planes)) {
			billboard++;
			num_visible_in_group++;
		}

		if (num_visible_in_group != 0) {
			memcpy(gpu_billboard_buffer_ptr + num_visible, initial_billboard, num_visible_in_group * sizeof(Billboard));
			num_visible += num_visible_in_group;
		}
	}

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
