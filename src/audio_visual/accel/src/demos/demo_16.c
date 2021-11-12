#include "demo_11.c"
#include "demo_15.c"

// Objects in a skybox scene

GLuint skybox_shader, skybox_texture;

StateGL demo_16_init(void) {
	StateGL sgl = {.vertex_array = init_vao()};

	const plane_type_t origin[3] = {1, 2, 2}, size[3] = {1, 2, 3};
	plane_type_t* const cuboid_mesh = create_sector_mesh(origin, size);

	sgl.num_vertex_buffers = 2;
	sgl.vertex_buffers = init_vbos(sgl.num_vertex_buffers, cuboid_mesh, bytes_per_mesh, skybox_vertices, sizeof(skybox_vertices));
	glEnableVertexAttribArray(0);
	free(cuboid_mesh);

	sgl.shader_program = init_shader_program(demo_4_vertex_shader, demo_4_fragment_shader);
	sgl.num_textures = 1;
	sgl.textures = init_textures(sgl.num_textures, "../../../assets/walls/mesa.bmp", tex_repeating);

	skybox_shader = init_shader_program(demo_15_vertex_shader, demo_15_fragment_shader);
	skybox_texture = init_skybox_texture("assets/sky_2.bmp");

	glEnable(GL_DEPTH_TEST);
	
	return sgl;
}

void demo_16_drawer(const StateGL* const sgl) {
	static Camera camera;
	static byte first_call = 1;
	static GLint view_projection_id, model_view_projection_id;

	const GLuint sector_shader = sgl -> shader_program;

	if (first_call) {
		init_camera(&camera, (vec3) {0.0f, 0.0f, 0.0f});
		view_projection_id = glGetUniformLocation(skybox_shader, "view_projection");
		model_view_projection_id = glGetUniformLocation(sector_shader, "model_view_projection");
		first_call = 0;
	}

	update_camera(&camera);

	//////////

	glUseProgram(sector_shader);

	glBindBuffer(GL_ARRAY_BUFFER, sgl -> vertex_buffers[0]);
	glVertexAttribPointer(0, 3, PLANE_TYPE_ENUM, GL_FALSE, bytes_per_vertex, NULL);
	glVertexAttribPointer(1, 2, PLANE_TYPE_ENUM, GL_FALSE, bytes_per_vertex, (void*) (3 * sizeof(plane_type_t)));

	glUniformMatrix4fv(model_view_projection_id, 1, GL_FALSE, &camera.model_view_projection[0][0]);

	select_texture_for_use(sgl -> textures[0], sector_shader);

	glEnableVertexAttribArray(1);
	draw_triangles(triangles_per_mesh);
	glDisableVertexAttribArray(1);

	//////////

	glDepthMask(GL_FALSE);
	glDepthFunc(GL_LEQUAL);

	glUseProgram(skybox_shader);

	glBindBuffer(GL_ARRAY_BUFFER, sgl -> vertex_buffers[1]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	const GLuint skybox_sampler_id = glGetUniformLocation(skybox_shader, "texture_sampler");
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_texture);
	glUniform1i(skybox_sampler_id, 0);

	camera.view_projection[3][0] = 0.0f;
	camera.view_projection[3][1] = 0.0f;
	camera.view_projection[3][3] = 0.0f;
	glUniformMatrix4fv(view_projection_id, 1, GL_FALSE, &camera.view_projection[0][0]);

	glDrawArrays(GL_TRIANGLES, 0, 36);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);
}

#ifdef DEMO_16
int main(void) {
	make_application(demo_16_drawer, demo_16_init, deinit_demo_vars);
}
#endif
