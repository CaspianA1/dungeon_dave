#include "demo_14.c"
#include "demo_15.c"

// Objects in a skybox scene

StateGL demo_16_init(void) {
	StateGL sgl = {.vertex_array = init_vao()};

	const mesh_type_t origin[3] = {1, 2, 2}, size[3] = {1, 2, 3};
	mesh_type_t* const cuboid_mesh = malloc(bytes_per_mesh);
	create_sector_mesh(origin, size, cuboid_mesh);

	sgl.num_vertex_buffers = 1;
	sgl.vertex_buffers = init_vbos(sgl.num_vertex_buffers, cuboid_mesh, bytes_per_mesh);
	free(cuboid_mesh);

	glEnableVertexAttribArray(0);

	sgl.shader_program = init_shader_program(demo_4_vertex_shader, demo_4_fragment_shader);

	sgl.num_textures = 1;
	sgl.textures = init_plain_textures(sgl.num_textures, "../../../../assets/walls/mesa.bmp", TexRepeating);
	glUseProgram(sgl.shader_program);
	use_texture(sgl.textures[0], sgl.shader_program, "texture_sampler", TexPlain, SECTOR_FACE_TEXTURE_UNIT);

	const Skybox skybox = init_skybox("../assets/sky_2.bmp");
	sgl.any_data = malloc(sizeof(Skybox));
	memcpy(sgl.any_data, &skybox, sizeof(Skybox));

	glEnable(GL_DEPTH_TEST);

	return sgl;
}

void demo_16_drawer(const StateGL* const sgl) {
	static Camera camera;
	static GLint model_view_projection_id;
	static bool first_call = true;

	const GLuint shader = sgl -> shader_program;

	if (first_call) {
		init_camera(&camera, (vec3) {0.0f, 0.0f, 0.0f});
		INIT_UNIFORM(model_view_projection, shader);
		first_call = false;
	}

	update_camera(&camera, get_next_event(), NULL);

	glUseProgram(shader);
	glBindBuffer(GL_ARRAY_BUFFER, sgl -> vertex_buffers[0]);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 3, MESH_TYPE_ENUM, GL_FALSE, bytes_per_vertex, NULL);
	glVertexAttribPointer(1, 2, MESH_TYPE_ENUM, GL_FALSE, bytes_per_vertex, (void*) (3 * sizeof(mesh_type_t)));

	UPDATE_UNIFORM(model_view_projection, Matrix4fv, 1, GL_FALSE, &camera.model_view_projection[0][0]);

	glDrawArrays(GL_TRIANGLES, 0, triangles_per_mesh * 3);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);

	draw_skybox(*((Skybox*) sgl -> any_data), &camera);
}

#ifdef DEMO_16
int main(void) {
	make_application(demo_16_drawer, demo_16_init, demo_15_deinit);
}
#endif
