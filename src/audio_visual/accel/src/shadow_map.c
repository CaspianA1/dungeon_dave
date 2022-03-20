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
- The gaussian blur process is pretty slow
- It seems like you can render into any output texture, which doesn't make any sense (perhaps both are rendered to?)
- Doing just one gaussian blur step does nothing
- Map edges cast odd shadows
- Contact hardening?
- Compute only one exponent for less space usage?
_____

Current framebuffer ping pong process:
1. Render scene, capturing depth moments in the first of the ping-pong textures.

2. Then, a gaussian blur process:
	- First read from the first texture, and blur that horizontally, writing the output to the second texture.
	- Then read from the second texture, and blur that vertically to the first texture.

- Apply this blur process as much as needed.

So: render scene -> t0, blur t0 -> t1, blur t1 -> t0

Or, do ping-ponging differently? https://www.khronos.org/opengl/wiki/Memory_Model#Framebuffer_objects
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

	"out vec4 moments;\n"

	"uniform vec2 warp_exps;\n"

	"vec2 warp_depth(float depth) {\n"
		"return vec2(exp(warp_exps.x * depth), -exp(-warp_exps.y * depth));\n"
	"}\n"

	"void main(void) {\n"
		"vec2 warped_depth = warp_depth(gl_FragCoord.z);\n"

		"vec2 dx = dFdx(warped_depth), dy = dFdy(warped_depth);\n"
		"vec2 warped_depth_biased_and_squared = warped_depth * warped_depth + 0.25f * (dx * dx + dy * dy);\n"

		"moments = vec4(warped_depth, warped_depth_biased_and_squared);\n"
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

	"out vec4 blurred_moments;\n"

	"uniform bool blurring_horizontally;\n"
	"uniform vec2 texel_size;\n"
	"uniform sampler2D image_sampler;\n"

	"#define KERNEL_SIZE 4\n" // Derived from https://observablehq.com/@jobleonard/gaussian-kernel-calculater
	"const float weights[KERNEL_SIZE] = float[KERNEL_SIZE](0.214606428562373, 0.1898792328888381, 0.13151412084312236, 0.07130343198685299);\n"

	"void main(void) {\n"
		"int index_from_opp_state = int(!blurring_horizontally);\n"
		"float texel_size_on_blurring_axis = texel_size[index_from_opp_state];\n"

		"vec2 UV_offset = vec2(0.0f);\n"
		"UV_offset[index_from_opp_state] = texel_size_on_blurring_axis;\n"

		"blurred_moments = texture(image_sampler, fragment_UV) * weights[0];\n"

		"for (int i = 1; i < KERNEL_SIZE; i++) {\n"
			"blurred_moments += weights[i] * (\n"
				"texture(image_sampler, fragment_UV + UV_offset) +\n"
				"texture(image_sampler, fragment_UV - UV_offset)\n"
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

	////////// Generating a render buffer to act as a z-buffer

	GLuint depth_render_buffer;
	glGenRenderbuffers(1, &depth_render_buffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depth_render_buffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, shadow_map_width, shadow_map_height);

	////////// Initializing a pair of textures to hold the outputted moment texture, for a 2-texture pingpong blur process

	GLuint ping_pong_textures[2];

	for (byte i = 0; i < 2; i++) {
		const bool make_mipmap = (i == SHADOW_MAP_OUTPUT_TEXTURE_INDEX);

		ping_pong_textures[i] = preinit_texture(
			TexPlain, TexNonRepeating, OPENGL_SHADOW_MAP_MAG_FILTER,
			make_mipmap ? OPENGL_SHADOW_MAP_MIN_FILTER : OPENGL_SHADOW_MAP_MAG_FILTER);

		glTexImage2D(TexPlain, 0, MOMENT_TEXTURE_SIZED_FORMAT, shadow_map_width,
			shadow_map_height, 0, MOMENT_TEXTURE_FORMAT, GL_FLOAT, NULL);

		if (make_mipmap) glGenerateMipmap(TexPlain);
	}

	const GLuint framebuffer = init_framebuffer(2, ping_pong_textures, &depth_render_buffer);
	disable_current_framebuffer();

	////////// Defining shaders, and getting the light direction

	const GLuint
		depth_shader = init_shader_program(depth_vertex_shader, depth_fragment_shader),
		blur_shader = init_shader_program(blur_vertex_shader, blur_fragment_shader);

	vec3 light_dir;
	get_dir_in_2D_and_3D(hori_angle, vert_angle, (vec2) {0.0f, 0.0f}, light_dir);

	//////////

	return (ShadowMapContext) {
		.light_context = {
			.pos = {light_pos[0], light_pos[1], light_pos[2]},
			.dir = {light_dir[0], light_dir[1], light_dir[2]}
			// `model_view_projection` unset
		},

		.buffer_context = {
			.size = {shadow_map_width, shadow_map_height},
			.framebuffer = framebuffer,
			.ping_pong_textures = {ping_pong_textures[0], ping_pong_textures[1]}
		},

		.shadow_pass = {
			.depth_render_buffer = depth_render_buffer,
			.depth_shader = depth_shader,
			.INIT_UNIFORM(light_model_view_projection, depth_shader)
		},

		.blur_pass = {
			.blur_shader = blur_shader,
			.INIT_UNIFORM(blurring_horizontally, blur_shader)
		}
	};
}

void deinit_shadow_map_context(ShadowMapContext* const shadow_map_context) {
	deinit_textures(2, shadow_map_context -> buffer_context.ping_pong_textures);
	glDeleteRenderbuffers(1, &shadow_map_context -> shadow_pass.depth_render_buffer);

	glDeleteProgram(shadow_map_context -> shadow_pass.depth_shader);
	glDeleteProgram(shadow_map_context -> blur_pass.blur_shader);

	deinit_framebuffer(shadow_map_context -> buffer_context.framebuffer);
}

static void get_model_view_projection_matrix_for_shadow_map(
	const ShadowMapContext* const shadow_map_context,
	const byte map_size[2], mat4 model_view_projection) {

	(void) map_size;

	mat4 view, projection;
	// const GLfloat far_clip_dist = 70.0f, l = sqrtf(50.0f * 50.0f + 50.0f * 50.0f + 18.0f * 18.0f); // 25.0f;
	const GLfloat far_clip_dist = 57.45f, l = sqrtf(40.0f * 40.0f + 40.0f * 40.0f + 10.0f * 10.0f);
	// const GLfloat far_clip_dist = 20.0f, l = 30.0f;
	glm_look_anyup((GLfloat*) shadow_map_context -> light_context.pos, (GLfloat*) shadow_map_context -> light_context.dir, view);
	glm_ortho(-l, l, l, -l, constants.camera.clip_dists.near, far_clip_dist, projection);
	glm_mul(projection, view, model_view_projection);
}

static void blur_shadow_map(ShadowMapContext* const shadow_map_context) {
	////////// Setting up some local variables

	const GLint blurring_horizontally_id = shadow_map_context -> blur_pass.blurring_horizontally_id;

	const GLuint
		blur_shader = shadow_map_context -> blur_pass.blur_shader,
		*const ping_pong_textures = shadow_map_context -> buffer_context.ping_pong_textures;

	glUseProgram(blur_shader);

	//////////

	static bool first_call = 1;

	if (first_call) {
		const GLsizei* const shadow_map_size = shadow_map_context -> buffer_context.size;
		INIT_UNIFORM_VALUE(texel_size, blur_shader, 2f, 1.0f / shadow_map_size[0], 1.0f / shadow_map_size[1]);
		set_sampler_texture_unit_for_shader("image_sampler", blur_shader, SHADOW_MAP_TEXTURE_UNIT);
		first_call = 0;
	}

	set_current_texture_unit(SHADOW_MAP_TEXTURE_UNIT);
	glDisable(GL_DEPTH_TEST); // Testing depths from the depth render buffer is not needed

	for (byte i = 0; i < constants.shadow_mapping.num_blur_passes << 1; i++) {
		const byte src_texture_index = i & 1;
		const byte dest_texture_index = !src_texture_index;

		// The shader reads from `src_texture`, while the other texture is meanwhile written to.
		set_current_texture(TexPlain, ping_pong_textures[src_texture_index]);

		// For a pass's first horizontal blur step, you write to the second texture; otherwise, the first.
		glDrawBuffer(GL_COLOR_ATTACHMENT0 + dest_texture_index);

		UPDATE_UNIFORM(blurring_horizontally, 1i, dest_texture_index); // Setting the current horizontal/vertical blur state
		glDrawArrays(GL_TRIANGLE_STRIP, 0, corners_per_quad);
	}

	glGenerateMipmap(TexPlain); // At this point, the current bound texture will be the output texture
	glEnable(GL_DEPTH_TEST);
}

static void enable_rendering_to_shadow_map(ShadowMapContext* const shadow_map_context_ref, const byte map_size[2]) {
	// TODO: copy only variables needed for the shadow pass + the MVP matrix
	ShadowMapContext shadow_map_context = *shadow_map_context_ref;

	////////// These matrices are relative to the light

	get_model_view_projection_matrix_for_shadow_map(shadow_map_context_ref,
		map_size, shadow_map_context.light_context.model_view_projection);

	////////// Activate shader, update light mvp, bind framebuffer, resize viewport, clear buffers, and cull front faces

	const GLuint depth_shader = shadow_map_context.shadow_pass.depth_shader;
	glUseProgram(depth_shader);

	static bool first_call = 1;

	if (first_call) {
		INIT_UNIFORM_VALUE(warp_exps, depth_shader, 2fv, 1, constants.shadow_mapping.warp_exps);
		first_call = 0;
	}

	UPDATE_UNIFORM(shadow_map_context.shadow_pass.light_model_view_projection,
		Matrix4fv, 1, GL_FALSE, (GLfloat*) shadow_map_context.light_context.model_view_projection);

	use_framebuffer(shadow_map_context.buffer_context.framebuffer);

	// TODO: needed?
	glDrawBuffer(GL_COLOR_ATTACHMENT0); // Initial rendering of scene moments is written to the first texture

	glViewport(0, 0, shadow_map_context.buffer_context.size[0], shadow_map_context.buffer_context.size[1]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glCullFace(GL_FRONT);

	memcpy(shadow_map_context_ref -> light_context.model_view_projection,
		shadow_map_context.light_context.model_view_projection, sizeof(mat4));
}

static void disable_rendering_to_shadow_map(const int screen_size[2], ShadowMapContext* const shadow_map_context) {
	// Turning on backface culling, blurring the shadow map, resetting the viewport size, and unbinding the framebuffer
	glCullFace(GL_BACK); // This goes before `blur_shadow_map` because otherwise, it interferes with the blurring process
	blur_shadow_map(shadow_map_context);
	glViewport(0, 0, screen_size[0], screen_size[1]);
	disable_current_framebuffer();
}

void render_all_sectors_to_shadow_map(
	ShadowMapContext* const shadow_map_context,
	const BatchDrawContext* const sector_draw_context,
	const int screen_size[2], const byte map_size[2]) {

	glBindBuffer(GL_ARRAY_BUFFER, sector_draw_context -> buffers.gpu);

	////////// Filling the sector gpu buffer with all of the sector vertices

	GLint bytes_for_vertices;
	glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bytes_for_vertices);
	const GLsizei total_num_vertices = bytes_for_vertices / bytes_per_face * vertices_per_face;

	glBufferSubData(GL_ARRAY_BUFFER, 0, bytes_for_vertices, sector_draw_context -> buffers.cpu.data);

	//////////

	enable_rendering_to_shadow_map(shadow_map_context, map_size);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, MESH_COMPONENT_TYPENAME, GL_FALSE, bytes_per_face_vertex, (void*) 0);
	glDrawArrays(GL_TRIANGLES, 0, total_num_vertices);
	glDisableVertexAttribArray(0);

	disable_rendering_to_shadow_map(screen_size, shadow_map_context);
}

#endif
