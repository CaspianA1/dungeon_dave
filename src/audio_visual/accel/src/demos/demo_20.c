#include "../utils.c"
#include "../list.c"
#include "../drawable_set.c"
#include "../texture.c"
#include "../camera.c"
#include "../skybox.c"

// Batched + culled billboard drawing

/*
- To begin with, just draw all in oen big unbatched buffer
- An expanded init_texture_set that takes spritesheets too
- Do I really need batching for billboards, as there will be so many fewer of them?
- Why is so much GPU time being used? Even happens when nothing rendered
- And why does calling glUseProgram not each time result in nothing being rendered?
- SDL_GL_SwapWindow is the culprit
- And demo 17 uses close to 88% of gpu power now
- Updating MacOS is a good idea
- Instancing render order may be a problem
- Only using instancing for glVertexAttribDivisor
- Border problems + distance jitter. Ideal: like with demo 13.
- Perhaps set alpha in surface earlier to 0 when needed
- Alpha is binary, but with linear interpolation, things become tricky (https://community.khronos.org/t/blending-and-alpha-black-border/18755/2)
*/

const char* const batching_billboard_vertex_shader =
	"#version 330 core\n"

	"layout(location = 0) in uint in_texture_id;\n"
	"layout(location = 1) in vec2 billboard_size_world_space;\n"
	"layout(location = 2) in vec3 billboard_center_world_space;\n"

	"out float texture_id;\n"
	"out vec2 UV;\n"

	"uniform vec2 cam_right_xz_world_space;\n"
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

*const batching_billboard_fragment_shader =
    "#version 330 core\n"

	"in float texture_id;\n"
	"in vec2 UV;\n"

	"out vec4 color;\n"

	"uniform sampler2DArray texture_sampler;\n"

	"void main() {\n"
		"color = texture(texture_sampler, vec3(UV, texture_id));\n"
		"if (color.a < 0.3f) discard;\n"
	"}\n";

// This struct is perfectly aligned
typedef struct {
	bb_texture_id_t texture_id;
	bb_pos_component_t size[2], pos[3];
} Billboard;

void draw_billboards(const DrawableSet* const billboard_list, const Camera* const camera) {
	const GLuint
		vbo = billboard_list -> dbo,
		batching_billboard_shader = billboard_list -> shader;

	static byte first_call = 1;
	static GLint cam_right_id, view_projection_id;

	if (first_call) {
		cam_right_id = glGetUniformLocation(batching_billboard_shader, "cam_right_xz_world_space");
		view_projection_id = glGetUniformLocation(batching_billboard_shader, "view_projection");
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // This gives a bigger edge border
		// glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // This one has more distance jitter
		first_call = 0;
	}

	glUseProgram(batching_billboard_shader);
	glUniform2f(cam_right_id, camera -> right_xz[0], camera -> right_xz[1]);
	glUniformMatrix4fv(view_projection_id, 1, GL_FALSE, &camera -> view_projection[0][0]);

	//////////

	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	for (byte i = 0; i < 3; i++) {
		glEnableVertexAttribArray(i);
		glVertexAttribDivisor(i, 1);
	}

	glVertexAttribIPointer(0, 1, BB_TEXTURE_ID_TYPENAME, sizeof(Billboard), NULL);
	glVertexAttribPointer(1, 2, BB_POS_COMPONENT_TYPENAME, GL_FALSE, sizeof(Billboard), (void*) offsetof(Billboard, size));
	glVertexAttribPointer(2, 3, BB_POS_COMPONENT_TYPENAME, GL_FALSE, sizeof(Billboard), (void*) offsetof(Billboard, pos));

	glUseProgram(billboard_list -> shader);
	glEnable(GL_BLEND);
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 5); // Each billboard 4 vertices, and 2 billboards
	glDisable(GL_BLEND);

	for (byte i = 0; i < 3; i++) {
		glDisableVertexAttribArray(i);
		glVertexAttribDivisor(i, 0);
	}
}

// Billboards
DrawableSet init_billboard_list(const size_t num_billboards, ...) {
	DrawableSet billboard_list = {
		.objects = init_list(num_billboards, Billboard),
		.object_indices = init_list(num_billboards, buffer_index_t), // 1 index per billboard attribute set
		.shader = init_shader_program(batching_billboard_vertex_shader, batching_billboard_fragment_shader)
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

	return billboard_list;
}

StateGL demo_20_init(void) {
	StateGL sgl = {.vertex_array = init_vao(), .num_vertex_buffers = 0, .num_textures = 0};

	DrawableSet billboard_list = init_billboard_list(
		// texture_set, 1, (Billboard) {0, {1.0f, 1.3658536585365855f}, {0.0f, 0.0f, 0.0f}}
		5,
		(Billboard) {2, {1.0f, 1.0f}, {0.0f, 1.0f, 2.0f}},
		(Billboard) {0, {1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}},
		(Billboard) {1, {1.0f, 1.0f}, {0.0f, 1.1f, 0.0f}},
		(Billboard) {2, {1.0f, 1.0f}, {0.0f, 1.5f, 3.0f}},
		(Billboard) {0, {1.0f, 1.0f}, {0.0f, 2.0f, 4.0f}}
	);

	billboard_list.texture_set = init_texture_set(TexNonRepeating,
		// 328, 448, 1, "../../../assets/objects/doomguy.bmp"
		64, 64, 3, "../../../assets/objects/doomguy.bmp",
		"../../../assets/objects/teleporter.bmp",
		"../../../assets/objects/robot.bmp"
	);

	// TODO: free billboard list somewhere
	DrawableSet* const billboard_list_on_heap = malloc(sizeof(DrawableSet));
	*billboard_list_on_heap = billboard_list;
	sgl.any_data = billboard_list_on_heap;

	enable_all_culling();

	return sgl;
}

void demo_20_drawer(const StateGL* const sgl) {
	const DrawableSet* const billboard_list = (DrawableSet*) sgl -> any_data;

	static byte first_call = 1;
	static Camera camera;
	static Skybox skybox;

	if (first_call) {
		skybox = init_skybox("assets/oasis_upscaled.bmp"); // TODO: free skybox somewhere
		init_camera(&camera, (vec3) {0.0f, 1.5f, -2.5f});
		first_call = 0;
	}

	update_camera(&camera, get_next_event());
	draw_billboards(billboard_list, &camera);
	draw_skybox(skybox, &camera);
}

#ifdef DEMO_20
int main(void) {
	make_application(demo_20_drawer, demo_20_init, deinit_demo_vars);
}
#endif
