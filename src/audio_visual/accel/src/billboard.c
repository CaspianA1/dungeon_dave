#ifndef BILLBOARD_C
#define BILLBOARD_C

#include "headers/billboard.h"
#include "list.c"
#include "data/shaders.c"

static byte is_inside_plane(vec4 sphere, vec4 plane) {
	return (glm_vec3_dot(plane, sphere) - plane[3]) < sphere[3];
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

void draw_billboards(const BatchDrawContext* const draw_context, const Camera* const camera) {
	const GLuint
		billboard_gpu_buffer = draw_context -> object_buffers.gpu,
		shader = draw_context -> shader;

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

	glBindBuffer(GL_ARRAY_BUFFER, billboard_gpu_buffer);

	for (byte i = 0; i < 3; i++) {
		glEnableVertexAttribArray(i);
		glVertexAttribDivisor(i, 1);
	}

	glVertexAttribIPointer(0, 1, BB_TEXTURE_ID_TYPENAME, sizeof(Billboard), NULL);
	glVertexAttribPointer(1, 2, BB_POS_COMPONENT_TYPENAME, GL_FALSE, sizeof(Billboard), (void*) offsetof(Billboard, size));
	glVertexAttribPointer(2, 3, BB_POS_COMPONENT_TYPENAME, GL_FALSE, sizeof(Billboard), (void*) offsetof(Billboard, pos));

	glEnable(GL_BLEND);
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 1); // Each billboard 4 vertices, and 1 billboard
	glDisable(GL_BLEND);

	for (byte i = 0; i < 3; i++) {
		glDisableVertexAttribArray(i);
		glVertexAttribDivisor(i, 0);
	}
}

BatchDrawContext init_billboard_draw_context(const size_t num_billboards, ...) {
	BatchDrawContext draw_context = {
		.object_buffers.cpu = init_list(num_billboards, Billboard),
		.shader = init_shader_program(billboard_vertex_shader, billboard_fragment_shader)
	};

	///////////
	va_list args;
	va_start(args, num_billboards);
	for (size_t i = 0; i < num_billboards; i++)
		((Billboard*) (draw_context.object_buffers.cpu.data))[i] = va_arg(args, Billboard);
	va_end(args);
	///////////

	glGenBuffers(1, &draw_context.object_buffers.gpu);
	glBindBuffer(GL_ARRAY_BUFFER, draw_context.object_buffers.gpu);
	glBufferData(GL_ARRAY_BUFFER, num_billboards * sizeof(Billboard),
		draw_context.object_buffers.cpu.data, GL_STATIC_DRAW);
	
	draw_context.gpu_buffer_ptr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

	return draw_context;
}

#endif
