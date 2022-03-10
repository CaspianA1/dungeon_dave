#ifndef SHADOW_MAP_C
#define SHADOW_MAP_C

/*
- Shadows for billboards and the weapon (they should read from the shadow map, not affect it)
- Limit orthographic matrix size for shadow map to map size

View frustum calculation:
- Need to capture whole scene
- Want an angle to the light as well

- So, can calculate position from those constraints
- Meaning that eventually, no position passed in
- So, given two angles, create a big plane
- Check that that plane is tight with the world's edge
- Then, make a cuboid extending from that plane to capture the whole world

- Later, depending on the size of the frustum, change the size of the light texture
- Lots of light bleeding right now (fix through exponential shadow maps)
- The gaussian blur process is pretty slow
_____

For only two textures with pingponging:
- First, both ping pongs with the same fbo
- Then, one of the ping pongs or the moment texture gone

Unoptimized ping-pong, with 3 textures: moment -> pp1, pp1 -> pp2, pp2 -> pp1
Doing the thing above right now, but pp1 and pp2 share a fbo
Optimized idea, with 2 textures: moment -> pp1, pp1 -> moment

- Ideally, just 1 total framebuffer with 2 textures in it
- So first, 1 framebuffer specific to the blur pass, and then 1 framebuffer in total after (which is done)
*/

#include "headers/shadow_map.h"
#include "headers/constants.h"
#include "headers/buffer_defs.h"
#include "texture.c"
#include "camera.c"

// TODO: to shaders.c
const GLchar *const depth_vertex_shader =
	"#version 330 core\n"

	"layout(location = 0) in vec3 vertex_pos_world_space;\n"

	"uniform mat4 light_model_view_projection;\n"

	"void main(void) {\n"
		"gl_Position = light_model_view_projection * vec4(vertex_pos_world_space, 1.0f);\n"
	"}\n",

*const depth_fragment_shader =
	"#version 330 core\n"

	"out vec2 moments;\n"

	"void main(void) {\n"
		"moments.x = gl_FragCoord.z;\n"
		"vec2 partial_derivatives = vec2(dFdx(moments.x), dFdy(moments.x));\n"
		"moments.y = moments.x * moments.x + 0.25f * dot(partial_derivatives, partial_derivatives);\n"
	"}\n",

*const blur_vertex_shader =
	"#version 330 core\n"

	"out vec2 fragment_UV;\n"

	// Bottom left, bottom right, top left, top right
	"const vec2 screen_corners[4] = vec2[4](\n"
		"vec2(-1.0f, -1.0f), vec2(1.0f, -1.0f),\n"
		"vec2(-1.0f, 1.0f), vec2(1.0f, 1.0f)\n"
	");\n"

	"void main(void) {\n"
		"gl_Position = vec4(screen_corners[gl_VertexID], 0.0f, 1.0f);\n"
		"fragment_UV = gl_Position.xy * 0.5f + 0.5f;\n"
	"}\n",

*const blur_fragment_shader =
	"#version 330 core\n"

	"in vec2 fragment_UV;\n"

	"out vec2 blurred_moments;\n"

	"uniform bool blurring_horizontally;\n"
	"uniform vec2 texel_size;\n"
	"uniform sampler2D image_sampler;\n"

	"#define KERNEL_SIZE 5\n"

	"const float weights[KERNEL_SIZE] = float[5](0.227027f, 0.1945946f, 0.1216216f, 0.054054f, 0.016216f);\n"

	"void main(void) {\n"
		"blurred_moments = texture(image_sampler, fragment_UV).rg * weights[0];\n"

		"int index_from_opp_state = int(!blurring_horizontally);\n"
		"float texel_size_on_blurring_axis = texel_size[index_from_opp_state];\n"

		"vec2 UV_offset = vec2(0.0f);\n"
		"UV_offset[index_from_opp_state] = texel_size_on_blurring_axis;\n"

		"for (int i = 1; i < KERNEL_SIZE; i++) {\n"
			"blurred_moments += weights[i] * (\n"
				"texture(image_sampler, fragment_UV + UV_offset).rg +\n"
				"texture(image_sampler, fragment_UV - UV_offset).rg\n"
			");\n"

			"UV_offset[index_from_opp_state] += texel_size_on_blurring_axis;\n"
		"}\n"
	"}\n";

static GLuint init_framebuffer(
	const byte num_attached_color_textures,
	const GLuint* const attached_color_textures,
	const GLuint* const attached_depth_render_buffer) {

	GLuint framebuffer;
	glGenFramebuffers(1, &framebuffer);
	use_framebuffer(framebuffer);

	for (byte i = 0; i < num_attached_color_textures; i++)
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, attached_color_textures[i], 0);

	if (attached_depth_render_buffer != NULL)
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, *attached_depth_render_buffer);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		fail("make a framebuffer; framebuffer not complete", CreateFramebuffer);

	return framebuffer;
}

ShadowMapContext init_shadow_map_context(const GLsizei shadow_map_width,
	const GLsizei shadow_map_height, const vec3 light_pos,
	const GLfloat hori_angle, const GLfloat vert_angle) {

	////////// Defining shaders

	const GLuint
		depth_shader = init_shader_program(depth_vertex_shader, depth_fragment_shader),
		blur_shader = init_shader_program(blur_vertex_shader, blur_fragment_shader);

	////////// Generating a texture to hold the two moments

	// Nearest would work if this texture were alone, but ideally, it should be a part of the ping-ponging process
	GLuint moment_texture;
	glGenTextures(1, &moment_texture);
	set_current_texture(TexPlain, moment_texture);
	glTexParameteri(TexPlain, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(TexPlain, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(TexPlain, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(TexPlain, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(TexPlain, 0, MOMENT_TEXTURE_SIZED_FORMAT, shadow_map_width,
		shadow_map_height, 0, MOMENT_TEXTURE_FORMAT, GL_FLOAT, NULL);

	////////// Generating a render buffer to act as a z-buffer

	GLuint depth_render_buffer;
	glGenRenderbuffers(1, &depth_render_buffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depth_render_buffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, shadow_map_width, shadow_map_height);

	////////// Defining a framebuffer, and attaching moment texture + depth render buffer to it

	const GLuint framebuffer = init_framebuffer(1, &moment_texture, &depth_render_buffer);

	////////// Initializing what's needed for a gaussian blur pass on the generated variance shadow map

	GLuint ping_pong_textures[2];
	glGenTextures(2, ping_pong_textures);

	for (byte i = 0; i < 2; i++) {
		const GLuint ping_pong_texture = ping_pong_textures[i];

		set_current_texture(TexPlain, ping_pong_texture);

		const bool texture_should_be_mipmapped = i == SHADOW_MAP_BLUR_OUTPUT_TEXTURE_INDEX;

		glTexParameteri(TexPlain, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(TexPlain, GL_TEXTURE_MIN_FILTER, texture_should_be_mipmapped ? OPENGL_TEX_MIN_FILTER : GL_LINEAR);
		glTexParameteri(TexPlain, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(TexPlain, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glTexImage2D(TexPlain, 0, MOMENT_TEXTURE_SIZED_FORMAT, shadow_map_width,
			shadow_map_height, 0, MOMENT_TEXTURE_FORMAT, GL_FLOAT, NULL);

		if (texture_should_be_mipmapped) glGenerateMipmap(TexPlain);
	}

	const GLuint ping_pong_framebuffer = init_framebuffer(2, ping_pong_textures, NULL);

	disable_current_framebuffer();

	//////////

	vec3 light_dir;
	get_dir_in_2D_and_3D(hori_angle, vert_angle, (vec2) {0.0f, 0.0f}, light_dir);

	return (ShadowMapContext) {
		.light_context = {
			.pos = {light_pos[0], light_pos[1], light_pos[2]},
			.dir = {light_dir[0], light_dir[1], light_dir[2]}
			// `model_view_projection` unset
		},

		.buffer_size = {shadow_map_width, shadow_map_height},

		.shadow_pass = {
			.framebuffer = framebuffer,
			.moment_texture = moment_texture,
			.depth_render_buffer = depth_render_buffer,

			.depth_shader = depth_shader,
			.INIT_UNIFORM(light_model_view_projection, depth_shader)
		},

		.blur_pass = {
			.ping_pong_framebuffer = ping_pong_framebuffer,
			.ping_pong_textures = {ping_pong_textures[0], ping_pong_textures[1]},

			.blur_shader = blur_shader,
			.INIT_UNIFORM(blurring_horizontally, blur_shader)
		}
	};
}

void deinit_shadow_map_context(ShadowMapContext* const shadow_map_context) {
	deinit_texture(shadow_map_context -> shadow_pass.moment_texture);
	glDeleteRenderbuffers(1, &shadow_map_context -> shadow_pass.depth_render_buffer);
	deinit_framebuffer(shadow_map_context -> shadow_pass.framebuffer);
	glDeleteProgram(shadow_map_context -> shadow_pass.depth_shader);

	deinit_textures(2, shadow_map_context -> blur_pass.ping_pong_textures);
	deinit_framebuffer(shadow_map_context -> blur_pass.ping_pong_framebuffer);
	glDeleteProgram(shadow_map_context -> blur_pass.blur_shader);
}

static void get_model_view_projection_matrix_for_shadow_map(
	const ShadowMapContext* const shadow_map_context,
	const byte map_size[2], mat4 model_view_projection) {

	(void) map_size;

	mat4 view, projection;
	const GLfloat far_clip_dist = 70.0f, l = sqrtf(50.0f * 50.0f + 50.0f * 50.0f + 18.0f * 18.0f); // 25.0f;
	// const GLfloat far_clip_dist = 20.0f, l = 30.0f;
	glm_look_anyup((GLfloat*) shadow_map_context -> light_context.pos, (GLfloat*) shadow_map_context -> light_context.dir, view);
	glm_ortho(-l, l, l, -l, constants.camera.clip_dists.near, far_clip_dist, projection);
	glm_mul(projection, view, model_view_projection);
}

static void blur_shadow_map(ShadowMapContext* const shadow_map_context) {
	////////// Setting up some local variables

	const GLint blurring_horizontally_id = shadow_map_context -> blur_pass.blurring_horizontally_id;

	const GLuint
		orig_moment_texture = shadow_map_context -> shadow_pass.moment_texture,
		blur_shader = shadow_map_context -> blur_pass.blur_shader,
		ping_pong_framebuffer =  shadow_map_context -> blur_pass.ping_pong_framebuffer,
		*const ping_pong_textures = shadow_map_context -> blur_pass.ping_pong_textures;

	glUseProgram(blur_shader);

	//////////

	static bool first_call = 1;

	if (first_call) {
		const GLsizei* const shadow_map_size = shadow_map_context -> buffer_size;
		INIT_UNIFORM_VALUE(texel_size, blur_shader, 2f, 1.0f / shadow_map_size[0], 1.0f / shadow_map_size[1]);
		set_sampler_texture_unit_for_shader("image_sampler", blur_shader, SHADOW_MAP_GENERATION_TEXTURE_UNIT);
		first_call = 0;
	}

	set_current_texture_unit(SHADOW_MAP_GENERATION_TEXTURE_UNIT);
	use_framebuffer(ping_pong_framebuffer);

	for (byte i = 0; i < constants.num_shadow_map_blur_passes * 2; i++) {
		const byte src_texture_index = i & 1;
		const byte blurring_horizontally = !src_texture_index;

		// For a pass's first horizontal blur step, you write to the second texture; otherwise, the first
		glDrawBuffer(GL_COLOR_ATTACHMENT0 + blurring_horizontally);

		// The shader reads from `src_texture`, while the other texture is meanwhile written to.
		const GLuint src_texture = (i == 0) ? orig_moment_texture : ping_pong_textures[src_texture_index];
		set_current_texture(TexPlain, src_texture);

		UPDATE_UNIFORM(blurring_horizontally, 1i, blurring_horizontally); // Setting the current horizontal/vertical blur state
		glDrawArrays(GL_TRIANGLE_STRIP, 0, corners_per_quad);
	}

	glGenerateMipmap(TexPlain); // At this point, the current bound texture will be the output texture
	disable_current_framebuffer();
}

static void enable_rendering_to_shadow_map(ShadowMapContext* const shadow_map_context_ref, const byte map_size[2]) {
	// TODO: copy only variables needed for the shadow pass + the MVP matrix
	ShadowMapContext shadow_map_context = *shadow_map_context_ref;

	////////// These matrices are relative to the light

	get_model_view_projection_matrix_for_shadow_map(shadow_map_context_ref,
		map_size, shadow_map_context.light_context.model_view_projection);

	////////// Activate shader, update light mvp, bind framebuffer, resize viewport, clear buffers, and cull front faces

	glUseProgram(shadow_map_context.shadow_pass.depth_shader);

	UPDATE_UNIFORM(shadow_map_context.shadow_pass.light_model_view_projection,
		Matrix4fv, 1, GL_FALSE, (GLfloat*) shadow_map_context.light_context.model_view_projection);

	use_framebuffer(shadow_map_context.shadow_pass.framebuffer);
	glViewport(0, 0, shadow_map_context.buffer_size[0], shadow_map_context.buffer_size[1]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glCullFace(GL_FRONT);

	memcpy(shadow_map_context_ref -> light_context.model_view_projection,
		shadow_map_context.light_context.model_view_projection, sizeof(mat4));
}

static void disable_rendering_to_shadow_map(const int screen_size[2], ShadowMapContext* const shadow_map_context) {
	// Unbinding the moment framebuffer, turning on backface culling, blurring the shadow map, and resetting the viewport size
	disable_current_framebuffer();
	glCullFace(GL_BACK); // This goes before `blur_shadow_map` because otherwise, it interferes with the blurring process
	blur_shadow_map(shadow_map_context);
	glViewport(0, 0, screen_size[0], screen_size[1]);
}

void render_all_sectors_to_shadow_map(
	ShadowMapContext* const shadow_map_context,
	const BatchDrawContext* const sector_draw_context,
	const int screen_size[2], const byte map_size[2]) {

	glBindBuffer(GL_ARRAY_BUFFER, sector_draw_context -> buffers.gpu);

	GLint bytes_for_vertices;
	glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bytes_for_vertices);
	const GLsizei total_num_vertices = bytes_for_vertices / bytes_per_face * vertices_per_face;

	glBufferSubData(GL_ARRAY_BUFFER, 0, bytes_for_vertices, sector_draw_context -> buffers.cpu.data);

	enable_rendering_to_shadow_map(shadow_map_context, map_size);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, MESH_COMPONENT_TYPENAME, GL_FALSE, bytes_per_face_vertex, (void*) 0);
	glDrawArrays(GL_TRIANGLES, 0, total_num_vertices);
	glDisableVertexAttribArray(0);

	disable_rendering_to_shadow_map(screen_size, shadow_map_context);
}

#endif
