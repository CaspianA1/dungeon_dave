/*
- Shadow maps for this demo - no shadow volumes, after some thinking.
- Start with making a plain shadow map: https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
- After that, move onto an omnidirectional shadow map: https://learnopengl.com/Advanced-Lighting/Shadows/Point-Shadows
- First, get depth buffer rendering working, and then make shadows. Done.
- Next, test more elaborate objects to be shadowed. Done.
- Next, use sampler shadows with GL_TEXTURE_COMPARE_MODE for no branch in the fragment shader.
- Next, add point lights.
- Find out why not calling capture_depth_buffer still works (test this on Linux to see if it's specific to MacOS).
- Also, making the matrix to a perspective projection one doesn't make the light a spotlight
- Integrate some of this code into demo 17, perhaps, to see what happens
- It also seems like areas behind the light are lit up, which they shouldn't be
*/

#include "../utils.c"
#include "../texture.c"
#include "../camera.c"
#include "../event.c"

typedef struct {
	const GLint texture_size[2];
	GLint light_model_view_projection_id;
	const GLuint shader, texture;
	GLuint framebuffer;
} DepthBufferCapture;

typedef struct { // `obj` here just means the objects in the scene
	const GLuint obj_shader;
	GLuint obj_vbo;
	GLsizei num_obj_vertices;
	DepthBufferCapture depth_capture;
} SceneState;

// TODO: make a shadows.c later, and put these shaders in shaders.c
const GLchar *const demo_23_obj_vertex_shader =
	"#version 330 core\n"

	"layout(location = 0) in vec3 vertex_pos_world_space;\n"

	"out vec4 shadow_coord;\n"

	"uniform mat4 light_bias_model_view_projection, obj_model_view_projection;\n"

	"void main(void) {\n"
		"vec4 vertex_pos_world_space_4D = vec4(vertex_pos_world_space, 1.0f);\n"
		"gl_Position = obj_model_view_projection * vertex_pos_world_space_4D;\n"
		"shadow_coord = light_bias_model_view_projection * vertex_pos_world_space_4D;\n"
	"}\n",

*const demo_23_obj_fragment_shader =
	"#version 330 core\n"

	"in vec4 shadow_coord;\n"

	"out vec3 color;\n"

	"uniform sampler2D depth_map_sampler;\n"

	"const float bias = 0.005f;\n"

	"void main(void) {\n"
		"float visibility = 1.0f;\n"

		"if (textureProj(depth_map_sampler, shadow_coord.xyw).z < bias / shadow_coord.w)\n"
			"visibility = 0.5f;\n"

		"color = visibility * vec3(0.2f, 0.5f, 0.7f);\n"
	"}\n";

DepthBufferCapture init_depth_buffer_capture(const GLint texture_width, const GLint texture_height) {
	const GLchar *const depth_map_vertex_shader =
		"#version 330 core\n"

		"layout(location = 0) in vec3 obj_vertex_pos_world_space;\n"

		"uniform mat4 light_model_view_projection;\n"

		"void main(void) {\n"
			"gl_Position = light_model_view_projection * vec4(obj_vertex_pos_world_space, 1.0f);\n"
		"}\n",

	*const depth_map_fragment_shader =
		"#version 330 core\n"

		"void main(void) {}\n";

	DepthBufferCapture depth_capture = {
		.texture_size = {texture_width, texture_height},
		.shader = init_shader_program(depth_map_vertex_shader, depth_map_fragment_shader),
		.texture = preinit_texture(TexPlain, TexNonRepeating)
	};

	glTexImage2D(TexPlain, 0, GL_DEPTH_COMPONENT, texture_width, texture_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	INIT_UNIFORM(depth_capture.light_model_view_projection, depth_capture.shader);

	glGenFramebuffers(1, &depth_capture.framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, depth_capture.framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, TexPlain, depth_capture.texture, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		puts("Framebuffer not complete");

	return depth_capture;
}

void deinit_depth_buffer_capture(const DepthBufferCapture depth_capture) {
	glDeleteProgram(depth_capture.shader);
	deinit_texture(depth_capture.texture);
	glDeleteFramebuffers(1, &depth_capture.framebuffer);
}

void capture_depth_buffer(const DepthBufferCapture depth_capture, void (*const drawer) (const void* const),
	const void* const drawer_param, const mat4 light_model_view_projection) {

	glUseProgram(depth_capture.shader);
	UPDATE_UNIFORM(depth_capture.light_model_view_projection, Matrix4fv, 1, GL_FALSE, &light_model_view_projection[0][0]);

	glViewport(0, 0, depth_capture.texture_size[0], depth_capture.texture_size[1]);
	glBindFramebuffer(GL_FRAMEBUFFER, depth_capture.framebuffer); // No need to clear color + depth buffer
	drawer(drawer_param);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

//////////

GLuint init_demo_23_obj_vbo(GLsizei* const num_obj_vertices) {
	const GLint num_components_per_vertex = 3;
	const GLfloat plane_size[2] = {22.0f, 25.0f}; // X and Z

	#define OBJ_OFFSET(x, y, z) (x) + plane_size[0] * 0.5f, (y) + 2.0f, (z) + plane_size[1] * 0.5f

	const GLfloat vertices[] = {
		// Bottom left of flat plane
		plane_size[0], 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f,
		plane_size[0], 0.0f, plane_size[1],

		// Top right of flat plane
		0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, plane_size[1],
		plane_size[0], 0.0f, plane_size[1],

		// Little pyramid
		OBJ_OFFSET(-0.5f, 0.0f, -0.5f),
		OBJ_OFFSET(0.5f, 0.0f, -0.5f),
		OBJ_OFFSET(0.0f, 1.0f, 0.0f),

		OBJ_OFFSET(-0.5f, 0.0f, -0.5f),
		OBJ_OFFSET(-0.5f, 0.0f, 0.5f),
		OBJ_OFFSET(0.0f, 1.0f, 0.0f),

		OBJ_OFFSET(0.5f, 0.0f, 0.5f),
		OBJ_OFFSET(-0.5f, 0.0f, 0.5f),
		OBJ_OFFSET(0.0f, 1.0f, 0.0f),

		OBJ_OFFSET(0.5f, 0.0f, 0.5f),
		OBJ_OFFSET(0.5f, 0.0f, -0.5f),
		OBJ_OFFSET(0.0f, 1.0f, 0.0f),

		0.0f, 0.0f, plane_size[1] * 0.1f,
		plane_size[0], 0.0f, plane_size[1],
		plane_size[0] * 0.5f, 4.0f, plane_size[1]
	};

	#undef OBJ_OFFSET

	*num_obj_vertices = sizeof(vertices) / sizeof(vertices[0]) / num_components_per_vertex;

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, num_components_per_vertex, GL_FLOAT, GL_FALSE, 0, (void*) 0);

	return vbo;
}

StateGL demo_23_init(void) {
	StateGL sgl = {.vertex_array = init_vao(), .num_vertex_buffers = 0, .num_textures = 0};

	SceneState scene_state = {
		.obj_shader = init_shader_program(demo_23_obj_vertex_shader, demo_23_obj_fragment_shader),
		.depth_capture = init_depth_buffer_capture(128, 128)
	};

	scene_state.obj_vbo = init_demo_23_obj_vbo(&scene_state.num_obj_vertices);

	//////////

	sgl.any_data = malloc(sizeof(SceneState));
	memcpy(sgl.any_data, &scene_state, sizeof(SceneState));

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	return sgl;
}

void demo_23_draw_call(const void* const param) {
	const GLsizei num_obj_vertices = *((GLsizei*) param);
	glDrawArrays(GL_TRIANGLES, 0, num_obj_vertices);
}

void demo_23_drawer(const StateGL* const sgl) {
	glClearColor(0.5f, 0.0f, 0.0f, 0.0f);

	const SceneState scene_state = *((SceneState*) sgl -> any_data);
	const DepthBufferCapture depth_capture = scene_state.depth_capture;

	static Camera camera;
	static GLint obj_model_view_projection_id, light_bias_model_view_projection_id;
	static byte first_call = 1;

	if (first_call) {
		INIT_UNIFORM(obj_model_view_projection, scene_state.obj_shader);
		INIT_UNIFORM(light_bias_model_view_projection, scene_state.obj_shader);
		init_camera(&camera, (vec3) {0.0f, 1.0f, 0.0f});
		first_call = 0;
	}

	////////// Math

	const Event event = get_next_event();
	update_camera(&camera, event, NULL);

	// TODO: set these up properly in camera.c later
	mat4 light_view, light_projection, light_view_projection, light_model_view_projection;

	static vec3 light_pos = {[1] = 1.647446f, [2] = 10.147844f};
	light_pos[0] = sinf(SDL_GetTicks() / 200.0f) - 0.0f;
	get_view_matrix(
		light_pos,
		(vec3) {0.853554f, -0.382683f, 0.353553f},
		(vec3) {-0.382683f, 0.0f, 0.923880f},
		light_view
	);

	glm_perspective(constants.camera.init.fov, (GLfloat) event.screen_size[0] / event.screen_size[1],
		constants.camera.clip_dists.near, constants.camera.clip_dists.far, light_projection);

	glm_mul(light_projection, light_view, light_view_projection);
	glm_mul(light_view_projection, GLM_MAT4_IDENTITY, light_model_view_projection);

	const mat4 bias_matrix = {
		{0.5f, 0.0f, 0.0f, 0.0f},
		{0.0f, 0.5f, 0.0f, 0.0f},
		{0.0f, 0.0f, 0.5f, 0.0f},
		{0.5f, 0.5f, 0.5f, 1.0f}
	};

	mat4 light_bias_model_view_projection;
	glm_mul((vec4*) light_model_view_projection, (vec4*) bias_matrix, light_bias_model_view_projection);

	// Rendering to depth map from light position
	capture_depth_buffer(depth_capture, demo_23_draw_call, &scene_state.num_obj_vertices, light_model_view_projection);

	// Rendering the scene as normal

	glUseProgram(scene_state.obj_shader);
	UPDATE_UNIFORM(obj_model_view_projection, Matrix4fv, 1, GL_FALSE, &camera.model_view_projection[0][0]);
	UPDATE_UNIFORM(light_bias_model_view_projection, Matrix4fv, 1, GL_FALSE, &light_bias_model_view_projection[0][0]);

	use_texture(depth_capture.texture, scene_state.obj_shader, "depth_map_sampler", TexPlain, SHADOW_MAP_TEXTURE_UNIT);

	glViewport(0, 0, event.screen_size[0], event.screen_size[1]);
	glClear(GL_DEPTH_BUFFER_BIT);
	demo_23_draw_call(&scene_state.num_obj_vertices);
}

void demo_23_deinit(const StateGL* const sgl) {
	SceneState* const scene_state = ((SceneState*) sgl -> any_data);

	glDeleteProgram(scene_state -> obj_shader);
	glDeleteBuffers(1, &scene_state -> obj_vbo);
	deinit_depth_buffer_capture(scene_state -> depth_capture);

	free(scene_state);

	deinit_demo_vars(sgl);
}

#ifdef DEMO_23
int main(void) {
	make_application(demo_23_drawer, demo_23_init, demo_23_deinit);
}
#endif
