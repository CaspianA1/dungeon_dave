#include "../utils.c"
#include "../texture.c"
#include "../camera.c"

const char* const demo_19_vertex_shader =
	"#version 330 core\n"

	"layout(location = 0) in vec3 vertex_pos_world_space;\n"

	"out float brightness;\n"
	"out vec2 UV;\n"

	"uniform vec3 spin;\n"
	"uniform mat4 model_view_projection;\n"

	"void main() {\n"
		"vec3 spin_pos = vertex_pos_world_space + spin;\n"
		"gl_Position = model_view_projection * vec4(spin_pos, 1);\n"
		"brightness = distance(spin, vertex_pos_world_space) / 3.0f;\n"
		"UV = spin_pos.xz;\n"
	"}\n",

*const demo_19_fragment_shader =
    "#version 330 core\n"

	"in float brightness;\n"
	"in vec2 UV;\n"

	"out vec3 color;\n"

	"uniform sampler2D texture_sampler;\n"

	"void main() {\n"
		"color = texture(texture_sampler, UV).rgb * brightness;\n"
	"}\n";

enum {vertices = 18, vertex_comps = 3, members = vertices * vertex_comps};

GLbyte* create_pyramid(const GLbyte top[3]) {
	GLbyte* const shape = malloc(members * sizeof(GLbyte));

	const GLbyte top_x = top[0], top_y = top[1], top_z = top[2];

	const GLbyte vertices[members] = {
		0, 0, 0, // Bottom square 1
		2, 0, 0,
		0, 0, 2,

		2, 0, 0, // Bottom square 2
		2, 0, 2,
		0, 0, 2,

		top_x, top_y, top_z, // Front
		2, 0, 0,
		0, 0, 0,

		top_x, top_y, top_z, // Left
		2, 0, 2,
		2, 0, 0,

		top_x, top_y, top_z, // Back
		0, 0, 2,
		2, 0, 2,

		top_x, top_y, top_z, // Right
		0, 0, 0,
		0, 0, 2
	};

	memcpy(shape, vertices, sizeof(vertices));
	return shape;
}

StateGL demo_19_init(void) {
	StateGL sgl = {.vertex_array = init_vao()};

	GLbyte* const shape = create_pyramid((GLbyte[3]) {1, 1, 1});

	sgl.num_vertex_buffers = 1;
	sgl.vertex_buffers = init_vbos(sgl.num_vertex_buffers, shape, members * sizeof(GLbyte));

	free(shape);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_BYTE, GL_FALSE, 0, NULL);

	sgl.shader_program = init_shader_program(demo_19_vertex_shader, demo_19_fragment_shader);
	glUseProgram(sgl.shader_program);

	sgl.num_textures = 1;
	sgl.textures = init_plain_textures(sgl.num_textures, "../../../assets/walls/mesa.bmp", TexRepeating);
	use_texture(sgl.textures[0], sgl.shader_program, TexPlain);

	enable_all_culling();

	return sgl;
}

void demo_19_drawer(const StateGL* const sgl) {
	static Camera camera;
	static GLint spin_id, model_view_projection_id;
	static byte first_call = 1;

	if (first_call) {
		init_camera(&camera, (vec3) {0.0f, 0.0f, -1.5f});
		spin_id = glGetUniformLocation(sgl -> shader_program, "spin");
		model_view_projection_id = glGetUniformLocation(sgl -> shader_program, "model_view_projection");
		first_call = 0;
	}

	update_camera(&camera, get_next_event());

	static GLfloat spin[2], spin_input = 0.0f;
	glUniform3f(spin_id, spin[0], spin[0] * spin[1], spin[1]);
	glUniformMatrix4fv(model_view_projection_id, 1, GL_FALSE, &camera.model_view_projection[0][0]);

	glClearColor(fmodf(spin[0] / 2.0f, 1.0f), fmodf(spin[1] / 2.0f, 1.0f), fmodf(spin_input, 1.0f), 0.0f);
	glDrawArrays(GL_TRIANGLES, 0, vertices);

	spin[0] = cosf(spin_input);
	spin[1] = sinf(spin_input);
	spin_input += 0.06f;
}

#ifdef DEMO_19
int main(void) {
	make_application(demo_19_drawer, demo_19_init, deinit_demo_vars);
}
#endif
