#ifndef WEAPON_SPRITE_C
#define WEAPON_SPRITE_C

#include "headers/weapon_sprite.h"
#include "headers/constants.h"
#include "headers/texture.h"
#include "headers/shader.h"

/*
Details on weapon coordinate space transformations:
- Certain parameters define how the weapon swings back and forth

- Screen-space coordinates are generated from those
- Then, those coordinates are unprojected into world-space, and copied over to the vbo
- In the fragment shader, the world-space coordinates are then reprojected to screen-space to get gl_Position

- This screen -> world -> screen process is done, instead of doing screen -> screen, for a few reasons:
	1. World-space coordinates are needed for lighting calculations
	2. Getting world coordinates, and then going to screen-space after that
		(in the fragment shader), works better with the shadow mapping pipeline
*/

/* TODO:
Plan for getting cast shadows in weapon sprites:

For drawing the weapon normally:
- First, use vbo for storing vertices on GPU + vao
- No array uniforms supplying any coordinates

Later, when shadow casting:
- Bind vbo + vao
- World coordinates are in the vbo
- They then get turned into screen coordinates through the view projection matrix
*/

//////////

static void update_weapon_sprite_animation(WeaponSprite* const ws, const Event* const event) {
	buffer_size_t curr_frame = ws -> curr_frame;

	if (curr_frame == 0) {
		if (CHECK_BITMASK(event -> movement_bits, BIT_USE_WEAPON)) {
			ws -> cycle_base_time = SDL_GetTicks();
			curr_frame++;
		}
	}
	else update_animation_information(ws -> cycle_base_time, &curr_frame, ws -> animation);

	ws -> curr_frame = curr_frame;
}

////////// This part concerns the mapping from weapon sway -> screen corners -> world corners

// Given an input between 0 and 1, this returns the y-value of the top left side of a circle
static GLfloat circular_mapping_from_zero_to_one(const GLfloat x) {
	/*
	- Goal: a smooth mapping from 0 to 1
	- And the y-value of a circle from x = 0 to 1, when its center equals (1, 0), achieves just that

	Circle equation: x^2 + y^2 = 1
	Shifted 1 unit to the right: (x - 1)^2 + y^2 = 1
	y^2 = 1 - (x - 1)^2
	y = sqrt(1 - (x - 1)^2)
	*/

	const GLfloat x_minus_one = x - 1.0f;
	return sqrtf(1.0f - x_minus_one * x_minus_one);
}

static void get_sway(const GLfloat speed_xz_percent, GLfloat sway[2]) {
	const GLfloat smooth_speed_xz_percent = circular_mapping_from_zero_to_one(speed_xz_percent);

	const GLfloat
		time_pace = sinf((SDL_GetTicks() / 1000.0f) * PI / constants.weapon_sprite.time_for_half_movement_cycle),
		weapon_movement_magnitude = constants.weapon_sprite.max_movement_magnitude * smooth_speed_xz_percent;

	sway[0] = time_pace * weapon_movement_magnitude * 0.5f * smooth_speed_xz_percent; // From -magnitude / 2.0f to magnitude / 2.0f
	sway[1] = (fabsf(sway[0]) - weapon_movement_magnitude) * smooth_speed_xz_percent; // From 0.0f to -magnitude
}

static void get_screen_corners_from_sway(const WeaponSprite* const ws, const GLfloat sway[2],
	const GLint screen_size[2], vec2 screen_corners[corners_per_quad]) {

	const GLfloat weapon_size = ws -> size, inv_screen_aspect_ratio = (GLfloat) screen_size[1] / screen_size[0];

	const GLfloat
		sway_across = sway[0],
		down_term = (weapon_size - 1.0f) + sway[1],
		across_term = weapon_size * ws -> frame_width_over_height * inv_screen_aspect_ratio;

	screen_corners[0][0] = screen_corners[2][0] = sway_across - across_term;
	screen_corners[1][0] = screen_corners[3][0] = sway_across + across_term;
	screen_corners[0][1] = screen_corners[1][1] = down_term - weapon_size;
	screen_corners[2][1] = screen_corners[3][1] = down_term + weapon_size;
}

static void get_world_corners_from_screen_corners(const mat4 view_projection,
	const vec2 screen_corners[corners_per_quad], vec3 world_corners[corners_per_quad]) {

	const GLfloat ndc_dist = constants.weapon_sprite.ndc_dist_from_camera;
	const vec4 viewport = {-1.0f, -1.0f, 2.0f, 2.0f};

	mat4 inv_view_projection;
	glm_mat4_inv((vec4*) view_projection, inv_view_projection);

	for (byte i = 0; i < corners_per_quad; i++) {
		const GLfloat* const corner = screen_corners[i];
		glm_unprojecti((vec3) {corner[0], corner[1], ndc_dist}, inv_view_projection, (GLfloat*) viewport, world_corners[i]);
	}
}

//////////

WeaponSprite init_weapon_sprite(const GLfloat size, const GLfloat texture_rescale_factor,
	const GLfloat secs_for_frame, const AnimationLayout animation_layout) {

	/* It's a bit wasteful to load the surface in `init_texture_set`
	and here, but this makes the code much more readable. TODO: perhaps
	query data about the texture set to figure out the frame size, if possible. */

	SDL_Surface* const peek_surface = init_surface(animation_layout.spritesheet_path);

	const GLsizei frame_size[2] = {
		peek_surface -> w / animation_layout.frames_across,
		peek_surface -> h / animation_layout.frames_down
	};

	deinit_surface(peek_surface);

	return (WeaponSprite) {
		// TODO: for multiple weapons, share this shader
		.shader = init_shader(ASSET_PATH("shaders/weapon.vert"), NULL, ASSET_PATH("shaders/weapon.frag")),

		.texture = init_texture_set(TexNonRepeating,
			OPENGL_WEAPON_MAG_FILTER, OPENGL_WEAPON_MIN_FILTER, 0, 1,
			(GLsizei) (frame_size[0] * texture_rescale_factor),
			(GLsizei) (frame_size[1] * texture_rescale_factor),
			NULL, &animation_layout
		),

		.animation = {
			.texture_id_range = {.start = 0, .end = (buffer_size_t) animation_layout.total_frames},
			.secs_for_frame = secs_for_frame
		},

		.cycle_base_time = 0, .curr_frame = 0,
		.frame_width_over_height = (GLfloat) frame_size[0] / frame_size[1],
		.size = size
	};
}

void deinit_weapon_sprite(const WeaponSprite* const ws) {
	deinit_texture(ws -> texture);
	deinit_shader(ws -> shader);
}

void update_and_draw_weapon_sprite(WeaponSprite* const ws_ref, const Camera* const camera,
	const Event* const event, const CascadedShadowContext* const shadow_context) {

	update_weapon_sprite_animation(ws_ref, event);

	const WeaponSprite ws = *ws_ref;

	use_shader(ws.shader);
	static GLint
		frame_index_id, world_corners_id, view_projection_id,
		camera_view_id, light_view_projection_matrices_id;

	ON_FIRST_CALL(
		INIT_UNIFORM(frame_index, ws.shader);
		INIT_UNIFORM(world_corners, ws.shader);
		INIT_UNIFORM(view_projection, ws.shader);
		INIT_UNIFORM(camera_view, ws.shader);
		INIT_UNIFORM(light_view_projection_matrices, ws.shader);

		INIT_UNIFORM_VALUE(ambient, ws.shader, 1f, constants.lighting.ambient);

		const List* const split_dists = &shadow_context -> split_dists;
		INIT_UNIFORM_VALUE(cascade_split_distances, ws.shader, 1fv, (GLsizei) split_dists -> length, split_dists -> data);

		use_texture(ws.texture, ws.shader, "frame_sampler", TexSet, WEAPON_TEXTURE_UNIT);
		use_texture(shadow_context -> depth_layers, ws.shader, "shadow_cascade_sampler", TexSet, CASCADED_SHADOW_MAP_TEXTURE_UNIT);
	);

	const vec4* const view_projection = camera -> view_projection;

	GLfloat sway[2];
	get_sway(camera -> speed_xz_percent, sway);

	vec2 screen_corners[corners_per_quad];
	get_screen_corners_from_sway(ws_ref, sway, event -> screen_size, screen_corners);

	vec3 world_corners[corners_per_quad];
	get_world_corners_from_screen_corners(view_projection, screen_corners, world_corners);

	//////////

	UPDATE_UNIFORM(frame_index, 1ui, ws.curr_frame);
	UPDATE_UNIFORM(world_corners, 3fv, corners_per_quad, (GLfloat*) world_corners);
	UPDATE_UNIFORM(view_projection, Matrix4fv, 1, GL_FALSE, (GLfloat*) view_projection);

	////////// This little part concerns CSM

	UPDATE_UNIFORM(camera_view, Matrix4fv, 1, GL_FALSE, &camera -> view[0][0]);

	const List* const light_view_projection_matrices = &shadow_context -> light_view_projection_matrices;

	UPDATE_UNIFORM(light_view_projection_matrices, Matrix4fv,
		(GLsizei) light_view_projection_matrices -> length, GL_FALSE,
		light_view_projection_matrices -> data);

	//////////

	/* Not using alpha to coverage here b/c blending is guaranteed
	to be correct for the last-rendered weapon's z-depth of zero */
	WITH_BINARY_RENDER_STATE(GL_BLEND,
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, corners_per_quad);
	);
}

#endif
