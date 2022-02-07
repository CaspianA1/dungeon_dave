/*
- Shadow maps for this demo - no shadow volumes, after some thinking.
- Start with making a plain shadow map: https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
- After that, move onto an omnidirectional shadow map: https://learnopengl.com/Advanced-Lighting/Shadows/Point-Shadows
*/

#include "../utils.c"
#include "../texture.c"
#include "../camera.c"

typedef struct {
	GLuint
		depth_map_framebuffer, depth_map_texture,
		scene_vbo;

	buffer_size_t shadow_size[2];
} SceneState;

StateGL demo_23_init(void) {
	StateGL sgl = {.vertex_array = init_vao(), .num_vertex_buffers = 0, .num_textures = 0};

	SceneState scene_state = {.shadow_size = {1024, 1024}};
	glGenFramebuffers(1, &scene_state.depth_map_framebuffer);
	scene_state.depth_map_texture = preinit_texture(TexPlain, TexNonRepeating);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		scene_state.shadow_size[0], scene_state.shadow_size[1],
		0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);

	glBindFramebuffer(GL_FRAMEBUFFER, scene_state.depth_map_framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, TexPlain, scene_state.depth_map_texture, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//////////

	glGenBuffers(1, &scene_state.scene_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, scene_state.scene_vbo);

	//////////

	sgl.any_data = malloc(sizeof(SceneState));
	memcpy(sgl.any_data, &scene_state, sizeof(SceneState));

	return sgl;
}

void demo_23_drawer(const StateGL* const sgl) {
	static Camera camera;
	static byte first_call = 1;

	if (first_call) {
		init_camera(&camera, (vec3) {0.0f, 0.0f, 0.0f});
		first_call = 0;
	}
	update_camera(&camera, get_next_event(), NULL);

	const SceneState scene_state = *((SceneState*) sgl -> any_data);

	// Rendering to depth map
	glViewport(0, 0, scene_state.shadow_size[0], scene_state.shadow_size[1]);
	glBindFramebuffer(GL_FRAMEBUFFER, scene_state.depth_map_framebuffer);
	glClear(GL_DEPTH_BUFFER_BIT);

	// Render scene from light position

	// Rendering as scene with shadow mapping, using depth map
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, WINDOW_W, WINDOW_H);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBindTexture(TexPlain, scene_state.depth_map_texture);

	// Render scene here
}

void demo_23_deinit(const StateGL* const sgl) {
	SceneState* const scene_state = sgl -> any_data;

	glDeleteBuffers(1, &scene_state -> scene_vbo);
	glDeleteFramebuffers(1, &scene_state -> depth_map_framebuffer);
	glDeleteTextures(1, &scene_state -> depth_map_texture);
	free(scene_state);

	deinit_demo_vars(sgl);
}

#ifdef DEMO_23
int main(void) {
	make_application(demo_23_drawer, demo_23_init, demo_23_deinit);
}
#endif
