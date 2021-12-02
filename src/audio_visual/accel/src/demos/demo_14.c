#include "demo_13.c"

// This differs from demo 13 in that it uses the new camera system, and asserts that sprite clipping works

void demo_14_drawer(const StateGL* const sgl) {
	static Camera camera;
	static GLint billboard_center_id, cam_right_xz_id, view_projection_id, model_view_projection_id;
	static byte first_call = 1;

	const GLuint billboard_shader = sgl -> shader_program;

	if (first_call) {
		init_camera(&camera, (vec3) {7.0f, 5.0f, 10.0f});

		billboard_center_id = glGetUniformLocation(billboard_shader, "billboard_center_world_space");
		cam_right_xz_id = glGetUniformLocation(billboard_shader, "cam_right_xz_world_space");
		view_projection_id = glGetUniformLocation(billboard_shader, "view_projection");
		model_view_projection_id = glGetUniformLocation(sector_shader, "model_view_projection");

		glUseProgram(billboard_shader);
		glUniform2f(glGetUniformLocation(billboard_shader, "billboard_size_world_space"), 1.0f, 1.0f);

		first_call = 0;
	}

	glUseProgram(billboard_shader);
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
	glUniform3f(billboard_center_id, new_center[0], new_center[1], new_center[2]);

	update_camera(&camera);
	glClearColor(0.1f, 0.9f, 0.9f, 0.0f);

	//////////

	glUseProgram(sector_shader);
	glUniformMatrix4fv(model_view_projection_id, 1, GL_FALSE, &camera.model_view_projection[0][0]);

	use_texture(sgl -> textures[1], sector_shader, TexPlain);
	glBindBuffer(GL_ARRAY_BUFFER, sgl -> vertex_buffers[0]);
	draw_triangles(triangles_per_mesh);

	//////////
	glUseProgram(billboard_shader);

	glUniform2f(cam_right_xz_id, camera.right_xz[0], camera.right_xz[1]);
	glUniformMatrix4fv(view_projection_id, 1, GL_FALSE, &camera.view_projection[0][0]);

	use_texture(sgl -> textures[0], sgl -> shader_program, TexPlain);

	glEnable(GL_BLEND); // Blending on for billboard
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDisable(GL_BLEND);
}

#ifdef DEMO_14
int main(void) {
	make_application(demo_14_drawer, demo_13_init, deinit_demo_vars);
}
#endif
