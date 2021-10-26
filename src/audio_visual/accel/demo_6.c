#include "demo_5.c"

/*
Other stuff:
- vert planes facing the other direction
- pass triangle size to shader for differently sized objects
- send integral points to the gpu
- one big mesh, with a texture lookup system, or separate meshes + separate draw calls, with one texture per mesh?
- but first, worry about 1 texture working for all meshes
- indexed rendering after figuring out textures

Passing in rect sizes to shader:
- a uniform var rectSizes, with some variable size
- pass those as uniforms into the vertex shader
- then, make uv_for_rects non-const, and then fetch the right tex coordinates
*/

/*
0__1
|  /
| /
2/
*/

enum {plane_vertex_floats = 18, plane_uv_floats = 12};

const size_t
	plane_vertex_bytes = plane_vertex_floats * sizeof(GLfloat),
	plane_uv_bytes = plane_uv_floats * sizeof(GLfloat);

//////////

GLfloat* create_uv_for_plane(const int width, const int height) {
	const GLfloat uv[plane_uv_floats] = {
		0.0f, 0.0f,
		width, 0.0f,
		0.0f, height,

		0.0f, height,
		width, height,
		width, 0.0f
	};

	GLfloat* const uv_data = malloc(plane_uv_bytes);
	memcpy(uv_data, uv, plane_uv_bytes);
	return uv_data;
}

#define PLANE_CREATOR_NAME(type) create_##type##_plane

#define PLANE_CREATOR_SIGNATURE const ivec3 top_left_corner, const int size_hori,\
	const int size_vert, GLfloat* const vertex_dest

#define PLANE_CREATOR_FUNCTION(type) void PLANE_CREATOR_NAME(type)(PLANE_CREATOR_SIGNATURE)

PLANE_CREATOR_FUNCTION(vert) {
	const float left_x = top_left_corner[0], top_y = top_left_corner[1], z = top_left_corner[2];
	const float right_x = left_x + size_hori, bottom_y = top_y - size_vert;

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

PLANE_CREATOR_FUNCTION(hori) {
	const float left_x = top_left_corner[0], height = top_left_corner[1], depth_origin = top_left_corner[2];
	const float right_x = left_x + size_hori, largest_depth = depth_origin + size_vert;

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
	Hori, Vert
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

		void (*const plane_creator)(PLANE_CREATOR_SIGNATURE) =
			(plane_def.type == Hori) ? PLANE_CREATOR_NAME(hori) : PLANE_CREATOR_NAME(vert);

		plane_creator(plane_def.top_left_corner, plane_def.size_hori,
			plane_def.size_vert, all_vertex_data + i * plane_vertex_floats);
	}

	va_end(args);
	return all_vertex_data;
}

//////////

const char* const demo_6_vertex_shader =
	"#version 330 core\n"
	"layout(location = 0) in vec3 vertex_pos_model_space;\n"
	// "layout(location = 1) in vec2 vertexUV;\n"
	"out vec2 UV;\n"
	"uniform mat4 MVP;\n"

	"const float w = 50.0f, h = 5.0f;\n"

	"const vec2 uv_for_rects[6] = vec2[6] (\n"
		"vec2(0.0f, 0.0f), vec2(w, 0.0f), vec2(0.0f, h),\n"
		"vec2(0.0f, h), vec2(w, h), vec2(w, 0.0f)\n"
	");\n"

	"void main() {\n"
		""
		"gl_Position = MVP * vec4(vertex_pos_model_space, 1);\n"
		// "UV = vertexUV;\n"
		"UV = uv_for_rects[gl_VertexID % 6];\n"
	"}\n";

//////////

StateGL demo_6_init(void) {
	StateGL sgl;

	sgl.vertex_array = init_vao();
	sgl.index_buffer = init_ibo(demo_3_index_data, sizeof(demo_3_index_data));

	const int size_hori = 50, size_vert = 5, num_planes = 3;
	const ivec3 origin = {1, 1, 1};

	GLfloat
		*const plane_vertices = create_plane_mesh(num_planes,
			(PlaneDef) {Hori, {origin[0], origin[1], origin[2]}, size_hori, size_vert},
			(PlaneDef) {Vert, {origin[0], size_vert + origin[1], origin[2]}, size_hori, size_vert},
			(PlaneDef) {Vert, {origin[0], origin[1] + size_vert, size_vert + origin[2]}, size_hori, size_vert}
		);

		// *const uv_data = create_uv_for_plane(size_hori, size_vert);

	sgl.num_vertex_buffers = 1;
	sgl.vertex_buffers = init_vbos(sgl.num_vertex_buffers,
		plane_vertices, num_planes * plane_vertex_bytes);
		// uv_data, plane_uv_bytes);
	
	free(plane_vertices);
	// free(uv_data);

	sgl.shader_program = init_shader_program(demo_6_vertex_shader, demo_4_fragment_shader);

	sgl.num_textures = 1;
	sgl.textures = init_textures(sgl.num_textures, "assets/walls/dune.bmp");
	select_texture_for_use(sgl.textures[0], sgl.shader_program);

	enable_all_culling();
	
	return sgl;
}

#ifdef DEMO_6
int main(void) {
    make_application(demo_5_drawer, demo_6_init, deinit_demo_vars);
}
#endif
