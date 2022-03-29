#include "demo_13.c"

// This differs from demo 13 in that it uses the new camera system, and asserts that sprite clipping works

void demo_14_drawer(const StateGL* const sgl) {
	static Camera camera;

	static GLint
		billboard_center_world_space_id, billboard_right_xz_world_space_id,
		billboard_model_view_projection_id, model_view_projection_id; // Not called sector_model_view_projection_id for compatibility reasons

	const GLuint billboard_shader = sgl -> shader;
	use_shader(billboard_shader);

	static bool first_call = true;

	if (first_call) {
		init_camera(&camera, (vec3) {7.0f, 5.0f, 10.0f});

		INIT_UNIFORM(billboard_center_world_space, billboard_shader);
		INIT_UNIFORM(billboard_right_xz_world_space, billboard_shader);
		INIT_UNIFORM(billboard_model_view_projection, billboard_shader);
		INIT_UNIFORM(model_view_projection, sector_shader);
		INIT_UNIFORM_VALUE(billboard_size_world_space, billboard_shader, 2f, 1.0f, 1.0f);

		first_call = false;
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

	UPDATE_UNIFORM(billboard_center_world_space, 3fv, 1, new_center);

	update_camera(&camera, get_next_event(), NULL);

	WITH_VERTEX_ATTRIBUTE(false, 0, 3, MESH_TYPE_ENUM, bytes_per_vertex, 0,
		WITH_VERTEX_ATTRIBUTE(false, 1, 2, MESH_TYPE_ENUM, bytes_per_vertex, 3 * sizeof(mesh_type_t),
			use_shader(sector_shader);
			glBindBuffer(GL_ARRAY_BUFFER, sgl -> vertex_buffers[0]);
			UPDATE_UNIFORM(model_view_projection, Matrix4fv, 1, GL_FALSE, &camera.model_view_projection[0][0]);
			glDrawArrays(GL_TRIANGLES, 0, triangles_per_mesh * 3);
		);
	);

	WITH_BINARY_RENDER_STATE(GL_BLEND, // Blending on for billboard
		WITHOUT_BINARY_RENDER_STATE(GL_CULL_FACE,
			use_shader(billboard_shader);
			UPDATE_UNIFORM(billboard_right_xz_world_space, 2f, camera.right_xz[0], camera.right_xz[1]);
			UPDATE_UNIFORM(billboard_model_view_projection, Matrix4fv, 1, GL_FALSE, &camera.model_view_projection[0][0]);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		);
	);
}

#ifdef DEMO_14
int main(void) {
	make_application(demo_14_drawer, demo_13_init, deinit_demo_vars);
}
#endif
