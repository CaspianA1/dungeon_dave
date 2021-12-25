#include "../utils.c"
#include "../list.c"
#include "../drawable_set.c"
#include "../texture.c"
#include "../camera.c"
#include "../skybox.c"

// Batched + culled billboard drawing

/*
- To begin with, just draw all in one big unbatched buffer
- Batching after
- Only using instancing for glVertexAttribDivisor
*/

const char* const batching_billboard_vertex_shader =
	"#version 330 core\n"

	"layout(location = 0) in uint in_texture_id;\n"
	"layout(location = 1) in vec2 billboard_size_world_space;\n"
	"layout(location = 2) in vec3 billboard_center_world_space;\n"

	"out float texture_id;\n"
	"out vec2 UV;\n"

	"uniform vec2 right_xz_world_space;\n"
	"uniform mat4 view_projection;\n"

	"const vec2 vertices_model_space[4] = vec2[4](\n"
		"vec2(-0.5f, -0.5f), vec2(0.5f, -0.5f),\n"
		"vec2(-0.5f, 0.5f), vec2(0.5f, 0.5f)\n"
	");\n"

	"void main(void) {\n"
		"vec2 vertex_model_space = vertices_model_space[gl_VertexID];\n"
		"vec2 corner_world_space = vertex_model_space * billboard_size_world_space;\n"

		"vec3 vertex_world_space = billboard_center_world_space\n"
			"+ corner_world_space.x * vec3(right_xz_world_space, 0.0f).xzy\n"
			"+ vec3(0.0f, corner_world_space.y, 0.0f);\n"

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
		"if (color.a < 0.28f) discard;\n" // 0.28f empirically tested for best discarding of alpha value
	"}\n";

// This struct is perfectly aligned
typedef struct {
	bb_texture_id_t texture_id;
	bb_pos_component_t size[2], pos[3];
} Billboard;

/*
Sphere intersection with plane:

bool is_inside_plane(plane):
	return plane.getSignedDistanceToPlane(self.center) > -self.radius;

float getSignedDistanceToPlane(point):
	return dot(self.normal, point) - self.distance;
*/

typedef struct {
	const float radius;
	vec3 center;
} Sphere;

byte is_inside_plane(Sphere sphere, vec4 plane) {
	return (glm_vec3_dot(plane, sphere.center) - plane[3]) > -sphere.radius;
}

byte billboard_in_view_frustum(const Billboard billboard, vec4 frustum_planes[6]) {
	const float half_w = billboard.size[0] * 0.5f, half_h = billboard.size[1] * 0.5f;

	Sphere sphere = {
		.radius = sqrtf(half_w * half_w + half_h * half_h),
		.center = {billboard.pos[0], billboard.pos[1], billboard.pos[2]}
	};

	return
		is_inside_plane(sphere, frustum_planes[0]) && is_inside_plane(sphere, frustum_planes[1]) &&
		is_inside_plane(sphere, frustum_planes[2]) && is_inside_plane(sphere, frustum_planes[3]) &&
		is_inside_plane(sphere, frustum_planes[4]) && is_inside_plane(sphere, frustum_planes[5]);
}

void draw_billboards(const DrawableSet* const billboard_list, const Camera* const camera) {
	const GLuint
		vbo = billboard_list -> dbo,
		batching_billboard_shader = billboard_list -> shader;

	static byte first_call = 1;
	static GLint right_id, view_projection_id;

	if (first_call) {
		right_id = glGetUniformLocation(batching_billboard_shader, "right_xz_world_space");
		view_projection_id = glGetUniformLocation(batching_billboard_shader, "view_projection");
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		first_call = 0;
	}

	glUseProgram(batching_billboard_shader);
	glUniform2f(right_id, camera -> right_xz[0], camera -> right_xz[1]);
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
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 1); // Each billboard 4 vertices, and 1 billboard
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
		// .object_indices = init_list(num_billboards, buffer_index_t), // 1 index per billboard attribute set
		.shader = init_shader_program(batching_billboard_vertex_shader, batching_billboard_fragment_shader)
	};

	///////////
	va_list args;
	va_start(args, num_billboards);
	for (size_t i = 0; i < num_billboards; i++) ((Billboard*) (billboard_list.objects.data))[i] = va_arg(args, Billboard);
	va_end(args);
	///////////

	glGenBuffers(1, &billboard_list.dbo);
	glBindBuffer(GL_ARRAY_BUFFER, billboard_list.dbo);
	glBufferData(GL_ARRAY_BUFFER, num_billboards * sizeof(Billboard), billboard_list.objects.data, GL_STATIC_DRAW);

	return billboard_list;
}

StateGL demo_20_init(void) {
	StateGL sgl = {.vertex_array = init_vao(), .num_vertex_buffers = 0, .num_textures = 0};

	/* How would I update indices for a texture index? Perhaps mod it by the current time in some way;
	Or actually not b/c the texture index may also depend on an enemy state */

	// TODO: free this somewhere
	DrawableSet billboard_list = init_billboard_list(
		1,
		(Billboard) {0, {10.0f, 10.0f}, {0.0f, 0.0f, 0.0f}}

		/*
		(Billboard) {1, {1.0f, 1.0f}, {0.0f, 1.0f, 1.0f}},
		(Billboard) {2, {1.0f, 1.0f}, {0.0f, 1.0f, 2.0f}},
		(Billboard) {3, {1.0f, 1.0f}, {0.0f, 1.0f, 3.0f}},
		(Billboard) {4, {1.0f, 1.0f}, {0.0f, 1.0f, 4.0f}},
		(Billboard) {5, {1.0f, 1.0f}, {0.0f, 1.0f, 5.0f}},
		(Billboard) {6, {1.0f, 1.0f}, {0.0f, 1.0f, 6.0f}},
		(Billboard) {7, {1.0f, 1.0f}, {0.0f, 1.0f, 7.0f}},
		(Billboard) {8, {1.0f, 1.0f}, {0.0f, 1.0f, 8.0f}},
		(Billboard) {9, {1.0f, 1.0f}, {0.0f, 1.0f, 9.0f}}
		*/
	);

	billboard_list.texture_set = init_texture_set(TexNonRepeating, 3, 2, 512, 512,
		"../../../../assets/objects/hot_dog.bmp",
		// "../../../../assets/walls/hieroglyphics.bmp",

		"../../../../assets/objects/teleporter.bmp",
		"../../../../assets/objects/robot.bmp",

		"../../../../assets/spritesheets/metroid.bmp", 2, 2, 4,
		"../../../../assets/spritesheets/bogo.bmp", 2, 3, 6
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
		skybox = init_skybox("../assets/desert.bmp"); // TODO: free skybox somewhere
		init_camera(&camera, (vec3) {0.0f, 0.0f, 0.0f}); // 0.0f, 1.5, -2.5f
		first_call = 0;
	}

	update_camera(&camera, get_next_event());
	draw_skybox(skybox, &camera);
	draw_billboards(billboard_list, &camera);

	//////////

	/*
	const Billboard hot_dog = ((Billboard*) (billboard_list -> objects.data))[0];
	vec4 frustum_planes[6];
	glm_frustum_planes((vec4*) camera.view_projection, frustum_planes);
	DEBUG(billboard_in_view_frustum(hot_dog, frustum_planes), d);
	*/
}

#ifdef DEMO_20
int main(void) {
	make_application(demo_20_drawer, demo_20_init, deinit_demo_vars);
}
#endif
