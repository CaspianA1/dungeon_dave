#include "demo_13.c"
#include "../batch.c"
#include "../sector.c"
#include "../camera.c"

// This differs from demo 13 in that it uses the new camera system

void demo_14_drawer(const StateGL* const sgl) {
	static byte first_call = 1;
	static Camera camera;
	static GLint cam_right_xz_id, view_projection_id, model_view_projection_id;

	const GLuint billboard_shader = sgl -> shader_program;

	if (first_call) {
		init_camera(&camera, (vec3) {7.0f, 5.0f, 10.0f});

		cam_right_xz_id = glGetUniformLocation(billboard_shader, "cam_right_xz_world_space");
		view_projection_id = glGetUniformLocation(billboard_shader, "view_projection");
		model_view_projection_id = glGetUniformLocation(poly_shader, "model_view_projection");

		glUseProgram(billboard_shader);
		glUniform3f(glGetUniformLocation(billboard_shader, "billboard_center_world_space"), center[0], center[1], center[2]);
		glUniform2f(glGetUniformLocation(billboard_shader, "billboard_size_world_space"), 1.0f, 1.0f);

		first_call = 0;
	}

	update_camera(&camera);
	glClearColor(0.1f, 0.9f, 0.9f, 0.0f);

	//////////

	glUseProgram(poly_shader);
	glUniformMatrix4fv(model_view_projection_id, 1, GL_FALSE, &camera.model_view_projection[0][0]);

	select_texture_for_use(sgl -> textures[1], poly_shader);
	glBindBuffer(GL_ARRAY_BUFFER, sgl -> vertex_buffers[0]);
	bind_interleaved_planes_to_vao();
	draw_triangles(2);
	glDisableVertexAttribArray(0);

	//////////

	glEnable(GL_BLEND); // Blending on for billboard
	glUseProgram(billboard_shader);

	glUniform2f(cam_right_xz_id, camera.right_xz[0], camera.right_xz[1]);
	glUniformMatrix4fv(view_projection_id, 1, GL_FALSE, &camera.view_projection[0][0]);

	select_texture_for_use(sgl -> textures[0], sgl -> shader_program);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisable(GL_BLEND);
}

#ifdef DEMO_14
int main(void) {
	make_application(demo_14_drawer, demo_13_init, deinit_demo_vars);
}
#endif
