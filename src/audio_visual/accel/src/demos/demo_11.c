#include "demo_4.c"
#include "../sector_mesh.c"
#include "../camera.c"

// Demo 10, but just with triangles in correct winding order

StateGL demo_11_init(void) {
	StateGL sgl = {.vertex_array = init_vao()};

	const mesh_type_t origin[3] = {2, 2, 5}, size[3] = {3, 2, 8};
	// plane_type_t* const cuboid_mesh = create_sector_mesh(origin, size);
	mesh_type_t* const cuboid_mesh = malloc(bytes_per_mesh);
	create_sector_mesh(origin, size, cuboid_mesh);

	sgl.num_vertex_buffers = 1;
	sgl.vertex_buffers = init_vbos(sgl.num_vertex_buffers, cuboid_mesh, bytes_per_mesh);
	free(cuboid_mesh);
	bind_interleaved_planes_to_vao();

	sgl.shader_program = init_shader_program(demo_4_vertex_shader, demo_4_fragment_shader);
	sgl.num_textures = 1;
	sgl.textures = init_textures(sgl.num_textures, "../../../assets/walls/mesa.bmp", tex_repeating);
	select_texture_for_use(sgl.textures[0], sgl.shader_program);
	enable_all_culling();

	return sgl;
}

void demo_11_drawer(const StateGL* const sgl) {
	static Camera camera;
	static GLint model_view_projection_id;
	static byte first_call = 1;

	if (first_call) {
		init_camera(&camera, (vec3) {0.0f, 0.0f, 0.0f});
		model_view_projection_id = glGetUniformLocation(sgl -> shader_program, "model_view_projection");
		first_call = 0;
	}

	update_camera(&camera);
	glUniformMatrix4fv(model_view_projection_id, 1, GL_FALSE, &camera.model_view_projection[0][0]);
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f); // Dark blue
	draw_triangles(triangles_per_mesh);
}

#ifdef DEMO_11
int main(void) {
	make_application(demo_11_drawer, demo_11_init, deinit_demo_vars);
}
#endif
