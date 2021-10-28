#include "demo_6.c"

enum {opt_plane_floats = 12};
enum {opt_plane_vertex_bytes = opt_plane_floats * sizeof(GLfloat)};

PLANE_CREATOR_FUNCTION(vert_opt) {
	const GLfloat left_x = top_left_corner[0], top_y = top_left_corner[1], z = top_left_corner[2];
	const GLfloat right_x = left_x + size_hori, bottom_y = top_y - size_vert;

	const GLfloat vertices[opt_plane_floats] = {
		left_x, top_y, z,
		right_x, top_y, z,

		left_x, bottom_y, z,
		right_x, bottom_y, z
	};

	memcpy(vertex_dest, vertices, opt_plane_vertex_bytes);
}

/*
Indices:     0 1 2 3 4 5 ...
Triangles:  {0 1 2}
			  {1 2 3} drawing order is (2 1 3) to maintain proper winding
				{2 3 4}
				  {3 4 5} drawing order is (4 3 5) to maintain proper winding
*/

StateGL demo_7_init(void) {
	StateGL sgl;

	sgl.vertex_array = init_vao();

	enum {num_planes = 1, size_hori = 3, size_vert = 2};
	const ivec3 top_left_corner = {5, 5, 5};
	GLfloat plane_sizes[num_planes * 2] = {size_hori, size_vert};

	GLfloat* const plane_vertices = malloc(opt_plane_vertex_bytes);
	PLANE_CREATOR_NAME(vert_opt)(top_left_corner, size_hori, size_vert, plane_vertices);

	sgl.num_vertex_buffers = 1;
	sgl.vertex_buffers = init_vbos(sgl.num_vertex_buffers,
		plane_vertices, num_planes * opt_plane_vertex_bytes);

	free(plane_vertices);

	demo_6_init_shader_and_textures_and_culling(&sgl, num_planes, plane_sizes);

	return sgl;
}

void demo_7_drawer(const StateGL sgl) {
	move(sgl.shader_program);
	glClearColor(0.4f, 0.0f, 0.0f, 0.0f); // Dark blue
	bind_vbos_to_vao(sgl.vertex_buffers, sgl.num_vertex_buffers, 3);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	unbind_vbos_from_vao(sgl.num_vertex_buffers);
}

#ifdef DEMO_7
int main(void) {
	make_application(demo_7_drawer, demo_7_init, deinit_demo_vars);
}
#endif
