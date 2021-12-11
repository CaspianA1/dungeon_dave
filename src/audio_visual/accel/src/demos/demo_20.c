#include "../utils.c"
#include "../list.c"
#include "../drawable_set.c"
#include "../texture.c"

// Batched + culled billboard drawing

/*
- To begin with, just draw all in oen big unbatched buffer
- See if can reduce number of inputs to vertex shader
- Set vertex attribs correctly
*/

const char* const batched_billboard_vertex_shader =
	"#version 330 core\n"

	"layout(location = 0) in vec3 billboard_center_world_space;\n"
	"layout(location = 1) in vec2 cam_right_xz_world_space;\n"
	"layout(location = 2) in vec2 billboard_size_world_space;\n"
	"layout(location = 3) in uint in_texture_id;\n"

	"out float texture_id;\n"
	"out vec2 UV;\n"

	"uniform mat4 view_projection;\n"

	"const vec2 vertices_model_space[4] = vec2[4](\n"
		"vec2(-0.5f, -0.5f), vec2(0.5f, -0.5f),\n"
		"vec2(-0.5f, 0.5f), vec2(0.5f, 0.5f)\n"
	");\n"

	"const vec3 cam_up_world_space = vec3(0.0f, 1.0f, 0.0f);\n"

	"void main() {\n"
		"vec2 vertex_model_space = vertices_model_space[gl_VertexID];\n"

		"vec3 vertex_world_space = billboard_center_world_space\n"
			"+ vec3(cam_right_xz_world_space, 0.0f).xzy * vertex_model_space.x * billboard_size_world_space.x\n"
			"+ cam_up_world_space * vertex_model_space.y * billboard_size_world_space.y;\n"

		"gl_Position = view_projection * vec4(vertex_world_space, 1.0f);\n"

		"texture_id = in_texture_id;\n"
		"UV = vec2(vertex_model_space.x, -vertex_model_space.y) + 0.5f;\n"
	"}\n",

*const batched_billboard_fragment_shader =
    "#version 330 core\n"

	"in float texture_id;\n"
	"in vec2 UV;\n"

	"out vec4 color;\n"

	"uniform sampler2DArray texture_sampler;\n"

	"void main() {\n"
		"color = texture(texture_sampler, vec3(UV, texture_id));\n"
	"}\n";

typedef struct {
	bb_pos_component_t pos[3];
	bb_texture_id_t texture_id;
} Billboard;

// Billboards
DrawableSet init_billboard_list(const GLuint texture_set, const size_t num_billboards, ...) {
	DrawableSet billboard_list = {
		.objects = init_list(num_billboards, Billboard),
		.object_indices = init_list(num_billboards, buffer_index_t), // 1 index per billboard attribute set
		.shader = init_shader_program(batched_billboard_vertex_shader, batched_billboard_fragment_shader),
		.texture_set = texture_set
	};

	///////////

	const size_t billboard_bytes = num_billboards * sizeof(Billboard);
	Billboard* const cpu_billboard_data = malloc(billboard_bytes);
	va_list args;
	va_start(args, num_billboards);
	for (size_t i = 0; i < num_billboards; i++) cpu_billboard_data[i] = va_arg(args, Billboard);
	va_end(args);

	///////////

	glGenBuffers(1, &billboard_list.dbo);
	glBindBuffer(GL_ARRAY_BUFFER, billboard_list.dbo);
	glBufferData(GL_ARRAY_BUFFER, billboard_bytes, cpu_billboard_data, GL_STATIC_DRAW);

	free(cpu_billboard_data);

	///////////

	for (byte i = 0; i < 4; i++) {
		glEnableVertexAttribArray(i);
		glVertexAttribDivisor(i, 1);
	}

	// Next step: passing data

	/*
	glEnableVertexAttribArray(0);
	glVertexAttribDivisor(0, 1);
	glVertexAttribPointer(0, 3, BB_POS_COMPONENT_TYPENAME, GL_FALSE, sizeof(Billboard), NULL);

	glEnableVertexAttribArray(1);
	glVertexAttribDivisor(1, 1);
	glVertexAttribIPointer(1, 1, BB_TEXTURE_ID_TYPENAME, sizeof(Billboard), (void*) sizeof(bb_pos_component_t[3]));
	*/

	return billboard_list;
}

StateGL demo_20_init(void) {
	const StateGL sgl = {.vertex_array = init_vao(), .num_vertex_buffers = 0, .num_textures = 0};

	const GLuint texture_set = init_texture_set(TexNonRepeating,
		64, 64, 3, "../../../assets/objects/tomato.bmp",
		"../../../assets/objects/teleporter.bmp",
		"../../../assets/objects/robot.bmp"
	);

	const DrawableSet billboard_list = init_billboard_list(
		texture_set, 5,
		(Billboard) {{2.5f, 3.5f, 4.5f}, 0},
		(Billboard) {{0.5f, 1.0f, 2.0f}, 1},
		(Billboard) {{1.0f, 1.2f, 2.3f}, 2},
		(Billboard) {{2.0f, 1.8f, 2.6f}, 2},
		(Billboard) {{3.0f, 1.6f, 5.2f}, 0}
	);

	(void) billboard_list;

	// TODO: free billboard list somewhere

	return sgl;
}

void demo_20_drawer(const StateGL* const sgl) {
	(void) sgl;
	// Each billboard 4 vertices, and 5 billboards
	// glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 5);
}

#ifdef DEMO_20
int main(void) {
	make_application(demo_20_drawer, demo_20_init, deinit_demo_vars);
}
#endif
