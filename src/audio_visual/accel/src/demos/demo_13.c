/*
- Billboards
- Next step: set uniform vars
- http://www.opengl-tutorial.org/intermediate-tutorials/billboards-particles/billboards/#solution-3--the-fixed-size-3d-way
- https://github.com/opengl-tutorials/ogl/tree/master/tutorial18_billboards_and_particles
*/

#include "demo_12.c"

StateGL demo_13_init(void) {
	StateGL sgl = {.vertex_array = init_vao()};

	const plane_type_t origin[3] = {1, 3, 0}, size[3] = {1, 3, 2};
	const plane_type_t near_x = origin[0], top_y = origin[1], near_z = origin[2], size_y = size[1], size_z = size[2];
	const plane_type_t bottom_y = top_y - size_y, far_z = near_z + size_z;

	const plane_type_t triangle_vertices[] = {
		near_x, bottom_y, near_z, 0, size_y,
		near_x, top_y, far_z, size_z, 0,
		near_x, top_y, near_z, 0, 0,

		near_x, bottom_y, near_z, 0, size_y,
		near_x, bottom_y, far_z, size_z, size_y,
		near_x, top_y, far_z, size_z, 0
	};


	sgl.num_vertex_buffers = 1;
	sgl.vertex_buffers = init_vbos(sgl.num_vertex_buffers, triangle_vertices, sizeof(triangle_vertices));
	bind_interleaved_planes_to_vao();

	const char* const vertex_shader =
		"#version 330 core\n"
		"layout(location = 0) in vec3 vertex;\n"
		"layout(location = 1) in vec2 vertexUV;\n"

		"out vec2 UV;\n"

		"uniform vec2 billboard_size;\n"
		"uniform vec3 cam_right_world_space, cam_up_world_space, billboard_center_world_space;\n"
		"uniform mat4 VP;\n" // View-projection matrix

		"void main() {\n"
			"vec3 vertex_pos_world_space = billboard_center_world_space\n"
				"+ cam_right_world_space * vertex.x * billboard_size.x\n"
				"+ cam_up_world_space * vertex.y * billboard_size.y;\n"

			"gl_Position = VP * vec4(vertex_pos_world_space, 1.0f);\n"
			"UV = vertexUV;\n"
		"}\n";
	
	(void) vertex_shader;

	sgl.shader_program = init_shader_program(demo_4_vertex_shader, demo_4_fragment_shader);
	sgl.num_textures = 1;
	sgl.textures = init_textures(sgl.num_textures, "../../../assets/objects/tomato.bmp");
	select_texture_for_use(sgl.textures[0], sgl.shader_program);
	// enable_all_culling();

	return sgl;
}

void demo_13_drawer(const StateGL* const sgl) {
	move(sgl -> shader_program);
	glClearColor(0.2f, 0.8f, 0.5f, 0.0f); // Barf green
	draw_triangles(2);
}

#ifdef DEMO_13
int main(void) {
	make_application(demo_13_drawer, demo_13_init, deinit_demo_vars);
}
#endif
