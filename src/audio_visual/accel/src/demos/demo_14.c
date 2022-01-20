#include "demo_13.c"

// This differs from demo 13 in that it uses the new camera system, and asserts that sprite clipping works

void demo_14_drawer(const StateGL* const sgl) {
	static Camera camera;

	static GLint
		billboard_center_world_space_id, right_xz_world_space_id,
		view_projection_id, model_view_projection_id;

	const GLuint billboard_shader = sgl -> shader_program;
	glUseProgram(billboard_shader);

	static byte first_call = 1;

	if (first_call) {
		init_camera(&camera, (vec3) {7.0f, 5.0f, 10.0f});

		INIT_UNIFORM(billboard_center_world_space, billboard_shader);
		INIT_UNIFORM(right_xz_world_space, billboard_shader);
		INIT_UNIFORM(view_projection, billboard_shader);
		INIT_UNIFORM(model_view_projection, sector_shader);
		INIT_UNIFORM_VALUE(billboard_size_world_space, billboard_shader, 2f, 1.0f, 1.0f);

		first_call = 0;
	}

	const GLfloat bb_offset_step = 0.01f;
	static vec3 bb_center_offset = {0.0f, 0.0f, 0.0f};
	if (keys[SDL_SCANCODE_5]) bb_center_offset[0] += bb_offset_step;
	if (keys[SDL_SCANCODE_6]) bb_center_offset[0] -= bb_offset_step;
	if (keys[SDL_SCANCODE_7]) bb_center_offset[1] += bb_offset_step;
	if (keys[SDL_SCANCODE_8]) bb_center_offset[1] -= bb_offset_step;
	if (keys[SDL_SCANCODE_9]) bb_center_offset[2] += bb_offset_step;
	if (keys[SDL_SCANCODE_0]) bb_center_offset[2] -= bb_offset_step;

	vec3 new_center;
	glm_vec3_add(center, bb_center_offset, new_center);
	glUniform3f(billboard_center_world_space_id, new_center[0], new_center[1], new_center[2]);

	update_camera(&camera, get_next_event(), NULL);
	glClearColor(0.1f, 0.9f, 0.9f, 0.0f);

	//////////

	glUseProgram(sector_shader);
	glUniformMatrix4fv(model_view_projection_id, 1, GL_FALSE, &camera.model_view_projection[0][0]);

	glBindBuffer(GL_ARRAY_BUFFER, sgl -> vertex_buffers[0]);
	bind_sector_mesh_to_vao();
	glDrawArrays(GL_TRIANGLES, 0, triangles_per_mesh * 3);
	unbind_sector_mesh_from_vao();

	//////////
	glUseProgram(billboard_shader);

	glUniform2f(right_xz_world_space_id, camera.right_xz[0], camera.right_xz[1]);
	glUniformMatrix4fv(view_projection_id, 1, GL_FALSE, &camera.view_projection[0][0]);

	glEnable(GL_BLEND); // Blending on for billboard
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDisable(GL_BLEND);
}

#ifdef DEMO_14
int main(void) {
	make_application(demo_14_drawer, demo_13_init, deinit_demo_vars);
}
#endif
