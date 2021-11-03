/*
- Billboards
- Next step: set uniform vars
- http://www.opengl-tutorial.org/intermediate-tutorials/billboards-particles/billboards/#solution-3--the-fixed-size-3d-way
- https://github.com/opengl-tutorials/ogl/tree/master/tutorial18_billboards_and_particles
*/

/*
- Have 4 corners of sprite
- Can get right vec and up vec in shader

E = player vector
P = billboard vector
F = player-billboard-delta vector = P - E
R = right vector = global up vector crossed with F
U = up vector = F crossed with R

Top right corner = U + R
Top left corner = U - R
Bottom left corner = -U - R
Bottom right corner = R - U
*/

#include "demo_12.c"

typedef GLfloat billboard_type_t;
#define BILLBOARD_TYPE_ENUM GL_FLOAT

const size_t bytes_per_billboard_vertex = 5 * sizeof(billboard_type_t);

const char* const demo_13_vertex_shader =
	"#version 330 core\n"
	"layout(location = 0) in vec3 vertex_model_space;\n"
	"layout(location = 1) in vec2 vertexUV;\n"

	"out vec2 UV;\n"

	"uniform vec2 billboard_size;\n"
	"uniform vec3 billboard_center_world_space, cam_right_world_space, cam_up_world_space;\n"
	"uniform mat4 VP;\n" // View-projection matrix

	"void main() {\n"
		"vec3 vertex_pos_world_space = billboard_center_world_space\n"
			"+ cam_right_world_space * vertex_model_space.x * billboard_size.x\n"
			"+ cam_up_world_space * vertex_model_space.y * billboard_size.y;\n"

		"gl_Position = VP * vec4(vertex_model_space, 1.0f);\n"
	"}\n";

void demo_13_matrix_setup(const GLuint shader_program) {
	const GLuint
		billboard_size = glGetUniformLocation(shader_program, "billboard_size"),
		billboard_center = glGetUniformLocation(shader_program, "billboard_center_world_space"),
		cam_right = glGetUniformLocation(shader_program, "cam_right_world_space"),
		cam_up = glGetUniformLocation(shader_program, "cam_up_world_space"),
		view_projection_matrix = glGetUniformLocation(shader_program, "VP");

	(void) billboard_size;
	(void) billboard_center;
	(void) cam_right;
	(void) cam_up;
	(void) view_projection_matrix;
}

StateGL demo_13_init(void) {
	StateGL sgl = {.vertex_array = init_vao()};

	const billboard_type_t top_left_corner[3] = {2, 3, 1}, size[3] = {1, 2, 1};

	const billboard_type_t
		near_x = top_left_corner[0], top_y = top_left_corner[1], near_z = top_left_corner[2],
		size_y = size[1], size_z = size[2];

	const billboard_type_t bottom_y = top_y - size_y, far_z = near_z + size_z;

	const billboard_type_t vertices[20] = {
		near_x, top_y, near_z, 0, 0,
		near_x, top_y, far_z, size_z, 0,

		near_x, bottom_y, near_z, 0, size_y,
		near_x, bottom_y, far_z, size_z, size_y
	};

	sgl.num_vertex_buffers = 1;
	sgl.vertex_buffers = init_vbos(sgl.num_vertex_buffers, vertices, sizeof(vertices));

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, BILLBOARD_TYPE_ENUM, GL_FALSE, bytes_per_billboard_vertex, NULL);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, BILLBOARD_TYPE_ENUM, GL_FALSE, bytes_per_billboard_vertex, (void*) (3 * sizeof(billboard_type_t)));

	sgl.shader_program = init_shader_program(demo_4_vertex_shader, demo_4_fragment_shader);

	sgl.num_textures = 1;
	sgl.textures = init_textures(sgl.num_textures, "../../../assets/objects/tomato.bmp");
	select_texture_for_use(sgl.textures[0], sgl.shader_program);
	// enable_all_culling();

	return sgl;
}

void demo_13_drawer(const StateGL* const sgl) {
	// demo_13_matrix_setup(sgl -> shader_program);
	move(sgl -> shader_program);

	glClearColor(0.2f, 0.8f, 0.5f, 0.0f); // Barf green
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

#ifdef DEMO_13
int main(void) {
	make_application(demo_13_drawer, demo_13_init, deinit_demo_vars);
}
#endif
