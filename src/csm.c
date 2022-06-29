#ifndef CSM_C
#define CSM_C

#include "headers/csm.h"
#include "headers/constants.h"
#include "headers/shader.h"
#include "headers/texture.h"

#define CSM_SIZED_DEPTH_FORMAT GL_DEPTH_COMPONENT32 // TODO: make this a parameter

/*
https://learnopengl.com/Guest-Articles/2021/CSM

For later on:
- Writing to `num_cascades.geom` before any shaders are initialized
- Texel snapping
- Merging the master branch with this one
- A world-space approach to merging the AABB of the sub frustum box
	with PSRs, instead of defining a scale factor for the frustum
*/

////////// This part concerns getting the light view projection matrix of a camera sub frustum

static void get_camera_sub_frustum_corners_and_center(const Camera* const camera, const GLfloat near_clip_dist,
	const GLfloat far_clip_dist, vec4 camera_sub_frustum_corners[corners_per_frustum], vec4 camera_sub_frustum_center) {

	mat4 camera_sub_frustum_projection, camera_sub_frustum_view_projection, inv_camera_sub_frustum_view_projection;

	glm_perspective(camera -> angles.fov, camera -> aspect_ratio,
		near_clip_dist, far_clip_dist, camera_sub_frustum_projection);

	glm_mul(camera_sub_frustum_projection, (vec4*) camera -> view, camera_sub_frustum_view_projection);
	glm_mat4_inv(camera_sub_frustum_view_projection, inv_camera_sub_frustum_view_projection);

	glm_frustum_corners(inv_camera_sub_frustum_view_projection, camera_sub_frustum_corners);
	glm_frustum_center(camera_sub_frustum_corners, camera_sub_frustum_center);
}

static void get_light_view(const vec4 camera_sub_frustum_center, const vec3 dir_to_light, mat4 light_view) {
	vec3 light_eye; // TODO: give this a better name
	glm_vec3_add((GLfloat*) camera_sub_frustum_center, (GLfloat*) dir_to_light, light_eye);
	glm_lookat(light_eye, (GLfloat*) camera_sub_frustum_center, (vec3) {0.0f, 1.0f, 0.0f}, light_view);
}

static void get_light_projection(const vec4 camera_sub_frustum_corners[corners_per_frustum],
	const vec3 light_view_frustum_box_scale, const mat4 light_view, mat4 light_projection) {

	vec3 light_view_frustum_box[2];
	glm_frustum_box((vec4*) camera_sub_frustum_corners, (vec4*) light_view, light_view_frustum_box);

	for (byte i = 0; i < 3; i++) {
		const GLfloat scale = light_view_frustum_box_scale[i];
		const GLfloat one_over_scale = 1.0f / scale;

		GLfloat
			*const min = &light_view_frustum_box[0][i],
			*const max = &light_view_frustum_box[1][i];

		*min *= (*min < 0.0f) ? scale : one_over_scale;
		*max *= (*max < 0.0f) ? one_over_scale : scale;
	}

	glm_ortho_aabb(light_view_frustum_box, light_projection);
}


// This modifies `light_projection` to avoid shadow swimming
static void apply_texel_snapping(const GLsizei resolution[2], const mat4 light_view_projection, mat4 light_projection) {
	/*
	- First tried https://www.junkship.net/News/2020/11/22/shadow-of-a-doubt-part-2
	- Then settling with https://stackoverflow.com/questions/33499053/cascaded-shadow-map-shimmering for now (using bounding sphere, which doesn't match with mine)
	- Perhaps try https://dev.theomader.com/stable-csm/ later (actually not, since its ortho matrix is in screen-space)
	- Or https://www.gamedev.net/forums/topic/711114-minimizing-shadow-mapping-shimmer/5443275/?

	Second way only works for far-away shadows (shimmering for close ones over especially big terrains).
	For far-away shadows, shimmering doesn't happen for movement, but it does for turning.

	- Check out this, maybe: https://www.gamedev.net/forums/topic/673197-cascaded-shadow-map-shimmering-effect/
	- Or https://github.com/TheRealMJP/Shadows/blob/master/Shadows/MeshRenderer.cpp#L1492%E2%80%8B
	- Or ask on Stackoverflow
	*/

	const vec2 half_resolution = {resolution[0] * 0.5f, resolution[1] * 0.5f};
	vec2 shadow_origin, rounding_offset;

	glm_vec2_mul((GLfloat*) light_view_projection[3], (GLfloat*) half_resolution, shadow_origin);
	glm_vec2_sub((vec2) {roundf(shadow_origin[0]), roundf(shadow_origin[1])}, shadow_origin, rounding_offset);
	glm_vec2_div(rounding_offset, (GLfloat*) half_resolution, rounding_offset);

	GLfloat* const column = light_projection[3];
	glm_vec2_add(column, rounding_offset, column);
}

static void get_sub_frustum_light_view_projection_matrix(const Camera* const camera,
	const CascadedShadowContext* const shadow_context, const GLfloat near_clip_dist,
	const GLfloat far_clip_dist, mat4 light_view_projection) {

	vec4 camera_sub_frustum_corners[corners_per_frustum], camera_sub_frustum_center;

	get_camera_sub_frustum_corners_and_center(camera, near_clip_dist,
		far_clip_dist, camera_sub_frustum_corners, camera_sub_frustum_center);

	mat4 light_view, light_projection;

	get_light_view(camera_sub_frustum_center, shadow_context -> dir_to_light, light_view);
	get_light_projection(camera_sub_frustum_corners, shadow_context -> sub_frustum_scale, light_view, light_projection);

	glm_mul(light_projection, light_view, light_view_projection);
	apply_texel_snapping(shadow_context -> resolution, light_view_projection, light_projection);
	glm_mul(light_projection, light_view, light_view_projection);
}

//////////

static GLuint init_csm_depth_layers(const GLsizei width, const GLsizei height, const GLsizei num_layers) {
	const GLuint depth_layers = preinit_texture(TexSet, TexNonRepeating, OPENGL_SHADOW_MAP_MAG_FILTER, OPENGL_SHADOW_MAP_MIN_FILTER, true);
	glTexImage3D(TexSet, 0, CSM_SIZED_DEPTH_FORMAT, width, height, num_layers, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	return depth_layers;
}

static GLuint init_csm_framebuffer(const GLuint depth_layers) {
	GLuint framebuffer;

	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depth_layers, 0);
	glDrawBuffer(GL_NONE); // Not drawing into any color buffers
	glReadBuffer(GL_NONE); // Not reading from any color buffers

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		FAIL(CreateFramebuffer, "OpenGL error is '%s'", get_GL_error());

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return framebuffer;
}

CascadedShadowContext init_shadow_context(const vec3 dir_to_light, const vec3 sub_frustum_scale,
	const GLfloat far_clip_dist, const GLfloat linear_split_weight, const GLsizei resolution[3]) {

	const GLsizei
		width = resolution[0], height = resolution[1],
		num_layers = resolution[2];

	List
		split_dists = init_list((buffer_size_t) num_layers - 1, GLfloat),
		light_view_projection_matrices = init_list((buffer_size_t) num_layers, mat4);

	light_view_projection_matrices.length = light_view_projection_matrices.max_alloc;
	split_dists.length = split_dists.max_alloc;

	const GLfloat
		near_clip_dist = constants.camera.near_clip_dist,
		clip_dist_diff = far_clip_dist - constants.camera.near_clip_dist;

	//////////

	for (buffer_size_t i = 0; i < split_dists.length; i++) {
		const GLfloat layer_percent = (GLfloat) (i + 1) / num_layers;

		const GLfloat linear_dist = near_clip_dist + layer_percent * clip_dist_diff;
		const GLfloat log_dist = near_clip_dist * powf(far_clip_dist / near_clip_dist, layer_percent);

		((GLfloat*) split_dists.data)[i] = glm_lerp(log_dist, linear_dist, linear_split_weight);
	}

	//////////

	const GLuint depth_layers = init_csm_depth_layers(width, height, num_layers);

	return (CascadedShadowContext) {
		.depth_layers = depth_layers,
		.framebuffer = init_csm_framebuffer(depth_layers),

		.depth_shader = init_shader(ASSET_PATH("shaders/csm/depth.vert"),
			ASSET_PATH("shaders/csm/depth.geom"), ASSET_PATH("shaders/csm/depth.frag")
		),

		.resolution = {width, height},

		.dir_to_light = {dir_to_light[0], dir_to_light[1], dir_to_light[2]},
		.sub_frustum_scale = {sub_frustum_scale[0], sub_frustum_scale[1], sub_frustum_scale[2]},
		.split_dists = split_dists, .light_view_projection_matrices = light_view_projection_matrices
	};
}

void deinit_shadow_context(const CascadedShadowContext* const shadow_context) {
	deinit_list(shadow_context -> split_dists);
	deinit_list(shadow_context -> light_view_projection_matrices);
	deinit_shader(shadow_context -> depth_shader);
	deinit_texture(shadow_context -> depth_layers);
	glDeleteFramebuffers(1, &shadow_context -> framebuffer);
}

void draw_to_shadow_context(const CascadedShadowContext* const shadow_context, const Camera* const camera,
	const GLint screen_size[2], void (*const drawer) (const void* const), const void* const drawer_param) {

	const List
		*const light_view_projection_matrices = &shadow_context -> light_view_projection_matrices,
		*const split_dists = &shadow_context -> split_dists;

	const buffer_size_t num_cascades = light_view_projection_matrices -> length;

	const GLfloat far_clip_dist = camera -> far_clip_dist;

	////////// Getting the matrices needed

	for (buffer_size_t i = 0; i < num_cascades; i++) {
		GLfloat sub_near_clip, sub_far_clip;

		if (i == 0) {
			sub_near_clip = constants.camera.near_clip_dist;
			sub_far_clip = value_at_list_index(split_dists, i, GLfloat);
		}
		else {
			sub_near_clip = value_at_list_index(split_dists, i - 1, GLfloat);
			sub_far_clip = (i == num_cascades - 1) ? far_clip_dist : value_at_list_index(split_dists, i, GLfloat);
		}

		vec4* const matrix = ptr_to_list_index(light_view_projection_matrices, i);
		get_sub_frustum_light_view_projection_matrix(camera, shadow_context, sub_near_clip, sub_far_clip, matrix);
	}

	////////// Updating the light view projection matrices uniform

	const GLuint depth_shader = shadow_context -> depth_shader;
	use_shader(depth_shader);

	static GLint light_view_projection_matrices_id;
	ON_FIRST_CALL(INIT_UNIFORM(light_view_projection_matrices, depth_shader););

	UPDATE_UNIFORM(light_view_projection_matrices, Matrix4fv, (GLsizei)
		num_cascades, GL_FALSE, light_view_projection_matrices -> data);

	////////// Rendering to the cascades

	const GLsizei* const resolution = shadow_context -> resolution;
	glViewport(0, 0, resolution[0], resolution[1]);
	glBindFramebuffer(GL_FRAMEBUFFER, shadow_context -> framebuffer);
	glClear(GL_DEPTH_BUFFER_BIT);

	WITH_BINARY_RENDER_STATE(GL_DEPTH_CLAMP, drawer(drawer_param);); // Shadow pancaking

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, screen_size[0], screen_size[1]);
}

#endif
