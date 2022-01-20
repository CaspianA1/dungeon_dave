// This demo will not work anymore unless the SDL_GL multisampling code is commented out. Same for 7.

#include "demo_5.c"

/*
To figure out:
- create shader as format string w num planes, if shader kept
- diff textures for diff planes
- indexed rendering
- eliminate repeated corners, so no GL_TRIANGLES
- then, get UVs not from shader, after that has been done (pack UVs next to triangle data in vbo)
- demo 7
*/

enum {plane_vertex_floats = 18};
const size_t plane_vertex_bytes = plane_vertex_floats * sizeof(GLfloat);

//////////

#define PLANE_CREATOR_NAME(type) create_##type##_plane

#define PLANE_CREATOR_SIGNATURE const ivec3 top_left_corner, const int size_hori,\
	const int size_vert, GLfloat* const vertex_dest

#define PLANE_CREATOR_FUNCTION(type) void PLANE_CREATOR_NAME(type)(PLANE_CREATOR_SIGNATURE)

PLANE_CREATOR_FUNCTION(vert_1) { // Faces towards north and south
	const GLfloat left_x = top_left_corner[0], top_y = top_left_corner[1], z = top_left_corner[2];
	const GLfloat right_x = left_x + size_hori, bottom_y = top_y - size_vert;

	const GLfloat vertices[plane_vertex_floats] = {
		left_x, top_y, z,
		right_x, top_y, z,
		left_x, bottom_y, z,

		left_x, bottom_y, z,
		right_x, bottom_y, z,
		right_x, top_y, z,
	};

	memcpy(vertex_dest, vertices, plane_vertex_bytes);
}

PLANE_CREATOR_FUNCTION(vert_2) { // Faces towards east and west
	const float x = top_left_corner[0], top_y = top_left_corner[1], far_z = top_left_corner[2];
	const float bottom_y = top_y - size_vert, near_z = far_z + size_hori;

	const GLfloat vertices[plane_vertex_floats] = {
		x, top_y, far_z,
		x, top_y, near_z,
		x, bottom_y, far_z,

		x, bottom_y, far_z,
		x, bottom_y, near_z,
		x, top_y, near_z
	};

	memcpy(vertex_dest, vertices, plane_vertex_bytes);
}

PLANE_CREATOR_FUNCTION(hori) {
	const GLfloat left_x = top_left_corner[0], height = top_left_corner[1], depth_origin = top_left_corner[2];
	const GLfloat right_x = left_x + size_hori, largest_depth = depth_origin + size_vert;

	const GLfloat vertices[plane_vertex_floats] = {
		left_x, height, depth_origin,
		right_x, height, depth_origin,
		left_x, height, largest_depth,

		left_x, height, largest_depth,
		right_x, height, largest_depth,
		right_x, height, depth_origin
	};

	memcpy(vertex_dest, vertices, plane_vertex_bytes);
}

//////////

typedef enum {
	Hori, Vert_1, Vert_2
} PlaneType;

typedef struct {
	const PlaneType type;
	const ivec3 top_left_corner;
	const int size_hori, size_vert;
} PlaneDef;

// Plane type, top left corner (an ivec3), size hori, size vert
GLfloat* create_plane_mesh(const int num_planes, ...) {
	va_list args;
	va_start(args, num_planes);
	GLfloat* const all_vertex_data = malloc(plane_vertex_bytes * num_planes);

	for (int i = 0; i < num_planes; i++) {
		const PlaneDef plane_def = va_arg(args, PlaneDef);

		void (*plane_creator)(PLANE_CREATOR_SIGNATURE);

		switch (plane_def.type) {
			case Hori:
				plane_creator = PLANE_CREATOR_NAME(hori); break;
			case Vert_1:
				plane_creator = PLANE_CREATOR_NAME(vert_1); break;
			case Vert_2:
				plane_creator = PLANE_CREATOR_NAME(vert_2); break;
		}
		plane_creator(plane_def.top_left_corner, plane_def.size_hori,
			plane_def.size_vert, all_vertex_data + i * plane_vertex_floats);
	}

	va_end(args);
	return all_vertex_data;
}

//////////

const GLchar* const demo_6_vertex_shader =
	"#version 330 core\n"

	"layout(location = 0) in vec3 vertex_pos_world_space;\n"

	"out vec2 UV;\n"

	"uniform vec2 plane_sizes[4];\n"
	"uniform mat4 model_view_projection;\n"

	"const vec2 unscaled_plane_UV[6] = vec2[6] (\n"
		"vec2(0.0f, 0.0f), vec2(1.0f, 0.0f), vec2(0.0f, 1.0f),\n"
		"vec2(0.0f, 1.0f), vec2(1.0f, 1.0f), vec2(1.0f, 0.0f)\n"
	");\n"

	"void main() {\n"
		"gl_Position = model_view_projection * vec4(vertex_pos_world_space, 1);\n"

		"int UV_index = gl_VertexID % 6, plane_index = gl_VertexID / 6;\n"
		"UV = unscaled_plane_UV[UV_index] * plane_sizes[plane_index];\n"
	"}\n";

//////////

void demo_6_core_init_shader_and_textures_and_culling(StateGL* const sgl) {
	sgl -> shader_program = init_shader_program(demo_6_vertex_shader, demo_4_fragment_shader);
	glUseProgram(sgl -> shader_program);

	sgl -> num_textures = 1;
	sgl -> textures = init_plain_textures(sgl -> num_textures, "../../../../assets/walls/dune.bmp", TexRepeating);
	use_texture(sgl -> textures[0], sgl -> shader_program, "texture_sampler", TexPlain, 0);

	enable_all_culling();
}

void demo_6_init_shader_and_textures_and_culling(StateGL* const sgl, const int num_planes, const GLfloat* const plane_sizes) {
	demo_6_core_init_shader_and_textures_and_culling(sgl);
	const GLint INIT_UNIFORM(plane_sizes, sgl -> shader_program);
	UPDATE_UNIFORM(plane_sizes, 2fv, num_planes, plane_sizes);
}

StateGL demo_6_init(void) {
	StateGL sgl;

	sgl.vertex_array = init_vao();

	enum {size_hori = 8, size_vert = 5, num_planes = 4};
	const ivec3 origin = {1, 1, 1};
	enum {size_hori_2 = 2, size_vert_2 = 3, size_hori_3 = 5};

	const GLfloat plane_sizes[num_planes * 2] = {
		size_hori_2, size_vert_2, size_hori, size_vert, size_hori, size_vert, size_hori_3, size_vert
	};

	GLfloat
		*const plane_vertices = create_plane_mesh(num_planes,
			(PlaneDef) {Hori, {origin[0], origin[1], origin[2]}, size_hori_2, size_vert_2},
			(PlaneDef) {Vert_1, {origin[0], size_vert + origin[1], origin[2]}, size_hori, size_vert},
			(PlaneDef) {Vert_1, {origin[0], origin[1] + size_vert, size_vert + origin[2]}, size_hori, size_vert},
			(PlaneDef) {Vert_2, {9.0f, 6.0f, 1.0f}, size_hori_3, size_vert}
		);

	sgl.num_vertex_buffers = 1;
	sgl.vertex_buffers = init_vbos(sgl.num_vertex_buffers,
		plane_vertices, num_planes * plane_vertex_bytes);

	bind_vbos_to_vao(sgl.vertex_buffers, sgl.num_vertex_buffers, 3);

	free(plane_vertices);

	demo_6_init_shader_and_textures_and_culling(&sgl, num_planes, plane_sizes);

	return sgl;
}

void demo_6_drawer(const StateGL* const sgl) {
	move(sgl -> shader_program);
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f); // Dark blue
	enum {num_triangles = 8};
	glDrawArrays(GL_TRIANGLES, 0, num_triangles * 3);
}

#ifdef DEMO_6
int main(void) {
    make_application(demo_6_drawer, demo_6_init, deinit_demo_vars);
}
#endif
