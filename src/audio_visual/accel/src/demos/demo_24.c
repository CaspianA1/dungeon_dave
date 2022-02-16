/*
- Ambient occlusion!
- This draws a sector-based map, and applies ambient occlusion to it.
*/

#include "../utils.c"
#include "../batch_draw_context.c"
#include "../sector.c"
#include "../texture.c"
#include "../camera.c"
#include "../event.c"
#include "../data/maps.c"

//////////

typedef struct {
	const struct {const GLuint pos, normal, color;} textures;
	const GLuint framebuffer, depth_render_buffer;
} GBuffer;

typedef struct {
	const struct {const GLuint color, blur;} textures;
	const struct {const GLuint framebuffer;} framebuffers;
} SSAOOutputBuffer;

typedef struct {
	const GBuffer g_buffer;
	const SSAOOutputBuffer ssao_output_buffer;
} AmbientOcclusionContext;

typedef struct {
	List sectors;
	BatchDrawContext sector_draw_context;

	AmbientOcclusionContext ambient_occlusion_context;

	const GLuint lightmap_texture; // This is grayscale

	const byte* const heightmap;
	const byte map_size[2];
} SceneState;

//////////

GBuffer init_g_buffer(const GLsizei g_buffer_width, const GLsizei g_buffer_height) {
	enum {num_textures = 3};

	const GLint internal_texture_formats[num_textures] = {GL_RGBA16F, GL_RGBA16F, GL_RGBA};

	const GLenum
		color_channel_types[num_textures] = {GL_FLOAT, GL_FLOAT, GL_UNSIGNED_BYTE},
		color_attachments[num_textures] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};

	//////////

	GLuint textures[num_textures], framebuffer;

	glGenTextures(num_textures, textures);
	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

	//////////

	for (byte i = 0; i < num_textures; i++) {
		const GLuint texture = textures[i];
		glBindTexture(GL_TEXTURE_2D, texture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		if (i == 0) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}

		glTexImage2D(GL_TEXTURE_2D, 0, internal_texture_formats[i],
			g_buffer_width, g_buffer_height, 0, GL_RGBA, color_channel_types[i], NULL);

		glFramebufferTexture2D(GL_FRAMEBUFFER, color_attachments[i], GL_TEXTURE_2D, texture, 0);
	}

	//////////

	glDrawBuffers(num_textures, color_attachments);

	//////////

	GLuint depth_render_buffer;

	glGenRenderbuffers(1, &depth_render_buffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depth_render_buffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, g_buffer_width, g_buffer_height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_render_buffer);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		puts("Framebuffer not complete");

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//////////

	return (GBuffer) {
		.textures = {textures[0], textures[1], textures[2]},
		.framebuffer = framebuffer, .depth_render_buffer = depth_render_buffer
	};
}

void deinit_g_buffer(const GBuffer g_buffer) {
	(void) g_buffer;
}

AmbientOcclusionContext init_ambient_occlusion_context(const GLint screen_width, const GLint screen_height) {
	const GBuffer g_buffer = init_g_buffer(screen_width, screen_height);
	// TODO: initialize SSAO output buffer too, and free resources
	return (AmbientOcclusionContext) {g_buffer, {0}};
}

void deinit_ambient_occlusion_context(const AmbientOcclusionContext context) {
	(void) context;
}

//////////

StateGL demo_24_init(void) {
	StateGL sgl = {.vertex_array = init_vao(), .num_vertex_buffers = 0, .num_textures = 0};

	SceneState scene_state = {
		.ambient_occlusion_context = init_ambient_occlusion_context(WINDOW_W, WINDOW_H),
		.lightmap_texture = init_plain_texture("../assets/water.bmp", TexPlain, TexNonRepeating, OPENGL_GRAYSCALE_INTERNAL_PIXEL_FORMAT),
		.heightmap = (byte*) palace_heightmap,
		.map_size = {palace_width, palace_height},
	};

	init_sector_draw_context(&scene_state.sector_draw_context, &scene_state.sectors,
		scene_state.heightmap, (byte*) palace_texture_id_map, scene_state.map_size);
	
	scene_state.sector_draw_context.texture_set = init_texture_set(TexRepeating,
		11, 0, 128, 128,
		"../../../../assets/walls/sand.bmp", "../../../../assets/walls/pyramid_bricks_4.bmp",
		"../../../../assets/walls/marble.bmp", "../../../../assets/walls/hieroglyph.bmp",
		"../../../../assets/walls/window.bmp", "../../../../assets/walls/saqqara.bmp",
		"../../../../assets/walls/sandstone.bmp", "../../../../assets/walls/cobblestone_3.bmp",
		"../../../../assets/walls/horses.bmp", "../../../../assets/walls/mesa.bmp",
		"../../../../assets/walls/arthouse_bricks.bmp"
	);
	
	sgl.any_data = malloc(sizeof(SceneState));
	memcpy(sgl.any_data, &scene_state, sizeof(SceneState));

	enable_all_culling();
	glEnable(GL_MULTISAMPLE);

	return sgl;
}

void demo_24_drawer(const StateGL* const sgl) {
	const SceneState* const scene_state = (SceneState*) sgl -> any_data;

	static Camera camera;
	static byte first_call = 1;

	if (first_call) {
		init_camera(&camera, (vec3) {1.5f, 1.5f, 1.5f});
		first_call = 0;
	}

	update_camera(&camera, get_next_event(), NULL);

	glClearColor(0.4f, 0.4f, 0.4f, 0.0f); // Light gray

	// TODO: find some way to use these private functions in a reasonable way

	//////////

	const BatchDrawContext* const draw_context = &scene_state -> sector_draw_context;
	const List* const sectors = &scene_state -> sectors;

	const buffer_size_t num_visible_faces = fill_sector_vbo_with_visible_faces(draw_context, sectors, &camera);
	if (num_visible_faces != 0) {
		draw_sectors(draw_context, &camera, num_visible_faces,
			scene_state -> lightmap_texture, scene_state -> map_size);
	}

	//////////
}

void demo_24_deinit(const StateGL* const sgl) {
	const SceneState* const scene_state = (SceneState*) sgl -> any_data;

	deinit_ambient_occlusion_context(scene_state -> ambient_occlusion_context);
	deinit_texture(scene_state -> lightmap_texture);
	deinit_batch_draw_context(&scene_state -> sector_draw_context);
	deinit_list(scene_state -> sectors);
	free(sgl -> any_data);

	deinit_demo_vars(sgl);
}

#ifdef DEMO_24
int main(void) {
	make_application(demo_24_drawer, demo_24_init, demo_24_deinit);
}
#endif
