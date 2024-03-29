#include "rendering/entities/weapon_sprite.h"
#include "utils/macro_utils.h" // For `CHECK_BITMASK`
#include "utils/opengl_wrappers.h" // For `INIT_UNIFORM_ID`, `INIT_UNIFORM_VALUE`, and `UPDATE_UNIFORM`
#include "utils/shader.h" // For `init_shader`

/* The weapon sprite code can be a bit hard to understand in the big picture.
	Here's how it works over a game tick:

1. The weapon sprite is updated during `update_weapon_sprite`.
	Its current animation frame is first updated, and then its
	world space corners are updated. It gets its world space corners like this:

	A. First, based on some parameters related to the camera, it defines the screen-space
		positions of the weapon sprite. It will sway back and forth more when the player
		is running faster.

	B. Then, those screen-space coordinates are unprojected to world-space,
		and then those world-space coordinates are rotated on various axes,
		causing the weapon sprite to change its yaw and pitch depending on the camera's roll and pitch.

2. It is then drawn to the shadow context via `draw_weapon_sprite_to_shadow_context`.
	In that, the world-space corners on the CPU are copied over to a vertex buffer on the GPU.
	It is then drawn.

3. The opaque parts of it are then drawn to the depth buffer via `draw_weapon_sprite_for_depth_prepass`.

4. Finally, it is drawn to the default framebuffer in `draw_weapon_sprite`. No vertex buffer
or spec are used for this; the four corners are simply passed in as a uniform. I figured that
there wouldn't be much point of binding a vertex spec if the vertex count is known
ahead of time, and is very small; so this should make the code a bit simpler and marginally faster. */

/* TODO: fix this:
- When debugging and separating the weapon sprite from the position of the player (by not updating the corners), these two things can happen:
	1. If the near clip dist is way too large, the weapon grows in size a ton.
	2. If the near clip dist is way too small, the weapon appears warped, and disappears from sight too easily.

TODO: do a depth prepass for the weapon sprite (and perhaps the rest of the geometry too), to discard
	some on-screen fragments, if that helps performance on the terrain 2 level (only do it for fragments with an alpha of 1)
*/

//////////

static void update_weapon_sprite_animation(WeaponSpriteAnimationContext* const animation_context, const Event* const event) {
	GLfloat* const cycle_start_time = &animation_context -> cycle_start_time;
	texture_id_t* const texture_id = &animation_context -> texture_id;
	const GLfloat curr_time_secs = event -> curr_time_secs;

	bool* const is_currently_being_animated = &animation_context -> is_currently_being_animated;
	animation_context -> activated_in_this_tick = false;

	if (!*is_currently_being_animated && CHECK_BITMASK(event -> movement_bits, BIT_USE_WEAPON)) {
		animation_context -> activated_in_this_tick = true;
		*is_currently_being_animated = true;
		*cycle_start_time = curr_time_secs;
	}

	if (*is_currently_being_animated)
		*is_currently_being_animated =
			!update_animation_information(curr_time_secs,
				animation_context -> animation,
				&animation_context -> cycle_start_time,
				texture_id
			);
}

////////// This part concerns the mapping from weapon sway -> screen corners -> world corners -> rotated world corners

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

static void get_screen_corners(const WeaponSprite* const ws,
	const Event* const event, const GLfloat speed_xz_percent,
	vec2 screen_corners[corners_per_quad]) {

	const WeaponSpriteAppearanceContext* const appearance_context = &ws -> appearance_context;

	////////// Getting the sway

	const GLfloat
		smooth_speed_xz_percent = circular_mapping_from_zero_to_one(speed_xz_percent),
		half_movement_cycles_per_sec = appearance_context -> screen_space.half_movement_cycles_per_sec,
		max_movement_magnitude = appearance_context -> screen_space.max_movement_magnitude;

	const GLfloat
		time_pace = sinf(event -> curr_time_secs * PI * half_movement_cycles_per_sec),
		weapon_movement_magnitude = max_movement_magnitude * smooth_speed_xz_percent;

	const GLfloat sway_x = time_pace * weapon_movement_magnitude * 0.5f * smooth_speed_xz_percent; // From -magnitude / 2 to magnitude / 2
	const GLfloat sway_y = (fabsf(sway_x) - weapon_movement_magnitude) * smooth_speed_xz_percent; // From 0 to -magnitude

	////////// Getting the screen corners

	const GLfloat
		screen_space_size = appearance_context -> screen_space.size,
		sprite_frame_width_over_height = appearance_context -> screen_space.frame_width_over_height;

	const GLfloat
		down_term = (screen_space_size - 1.0f) + sway_y,
		across_term = screen_space_size * sprite_frame_width_over_height / event -> aspect_ratio;

	screen_corners[0][0] = screen_corners[2][0] = sway_x - across_term;
	screen_corners[1][0] = screen_corners[3][0] = sway_x + across_term;
	screen_corners[0][1] = screen_corners[1][1] = down_term - screen_space_size;
	screen_corners[2][1] = screen_corners[3][1] = down_term + screen_space_size;
}

static void get_world_corners(const vec2 screen_corners[corners_per_quad],
	const Camera* const camera, WeaponSpriteAppearanceContext* const appearance_context) {

	vec3* const world_corners = appearance_context -> world_space.corners;

	////////// Unprojecting the screen-space corners to get world-space corners

	const vec4* const view_projection = camera -> view_projection;
	const vec4 viewport = {-1.0f, -1.0f, 2.0f, 2.0f};

	mat4 inv_view_projection;
	glm_mat4_inv((vec4*) view_projection, inv_view_projection);

	for (byte i = 0; i < corners_per_quad; i++) {
		const GLfloat* const screen_corner = screen_corners[i];

		glm_unprojecti((vec3) {screen_corner[0], screen_corner[1], 0.0f},
			inv_view_projection, (GLfloat*) viewport, world_corners[i]);
	}

	////////// Getting the world-space size of the weapon

	GLfloat
		*const bottom_left = world_corners[0], *const bottom_right = world_corners[1],
		*const top_left = world_corners[2], *const top_right = world_corners[3];

	// TODO: why does this grow when increasing the FOV?
	const vec2 world_space_size = {
		glm_vec3_distance(bottom_left, bottom_right),
		glm_vec3_distance(bottom_left, top_left)
	};

	////////// Yaw

	const GLfloat camera_roll_percent = camera -> angles.tilt / constants.camera.limits.tilt_max;

	const GLfloat
		yaw_angle = -camera_roll_percent * appearance_context -> world_space.max_yaw,
		*const camera_right = camera -> right;

	vec3 yaw_offset;
	glm_vec3_scale((GLfloat*) camera_right, world_space_size[0], yaw_offset);
	glm_vec3_rotate(yaw_offset, yaw_angle, (GLfloat*) camera -> up);

	// Overwriting a old side edge with a rotated one
	if (yaw_angle > 0.0f) {
		glm_vec3_sub(bottom_right, yaw_offset, bottom_left);
		glm_vec3_sub(top_right, yaw_offset, top_left);
	}
	else {
		glm_vec3_add(bottom_left, yaw_offset, bottom_right);
		glm_vec3_add(top_left, yaw_offset, top_right);
	}

	////////// Pitch

	const GLfloat camera_pitch_percent = camera -> angles.vert / constants.camera.limits.vert_max;
	const GLfloat pitch_angle = camera_pitch_percent * appearance_context -> world_space.max_pitch;

	vec3 pitch_offset = {0.0f, world_space_size[1], 0.0f};
	glm_vec3_rotate(pitch_offset, pitch_angle, (GLfloat*) camera_right);

	// Overwriting the top edge with one that has pitch applied
	glm_vec3_add(bottom_left, pitch_offset, top_left);
	glm_vec3_add(bottom_right, pitch_offset, top_right);
}

static void get_quad_tbn_matrix(const vec3* const quad_corners, mat3 tbn) {
	GLfloat *const tangent = tbn[0], *const bitangent = tbn[1], *const normal = tbn[2];
	const GLfloat* const bl_corner = quad_corners[0];

	glm_vec3_sub((GLfloat*) bl_corner, (GLfloat*) quad_corners[1], tangent);
	glm_vec3_normalize(tangent); // Flows along S

	glm_vec3_sub((GLfloat*) quad_corners[2], (GLfloat*) bl_corner, bitangent);
	glm_vec3_normalize(bitangent); // Flows along T

	// Since the tangent and bitangent are orthogonal and normalized, this will also be normalized
	glm_vec3_cross(bitangent, tangent, normal);
}

////////// This part is for the uniform updater param type and the uniform updater

typedef struct {const WeaponSprite* const weapon_sprite;} UniformUpdaterParams;

static void update_uniforms(const void* const param) {
	const WeaponSprite* const ws = (*(UniformUpdaterParams*) param).weapon_sprite;
	const vec3* const world_corners = ws -> appearance_context.world_space.corners;

	mat3 tbn;
	get_quad_tbn_matrix(world_corners, tbn);

	UPDATE_UNIFORM(ws, world_space, frame_index, 1ui, ws -> animation_context.texture_id);
	UPDATE_UNIFORM(ws, world_space, world_corners, 3fv, corners_per_quad, (GLfloat*) world_corners);
	UPDATE_UNIFORM(ws, world_space, tbn, Matrix3fv, 1, GL_FALSE, (GLfloat*) tbn);
}

////////// Initialization, deinitialization, updating, and rendering

static void define_vertex_spec(void) {
	define_vertex_spec_index(false, true, 0, 3, 0, 0, GL_FLOAT);
}

WeaponSprite init_weapon_sprite(
	const WeaponSpriteConfig* const config,
	const NormalMapCreator* const normal_map_creator,
	const material_index_t material_index) {

	////////// Getting the frame size and an albedo texture set

	const AnimationLayout* const animation_layout = &config -> animation_layout;

	/* It's a bit wasteful to load the surface in `init_texture_set`
	and here too, but this makes the code much more readable. */
	SDL_Surface* const peek_surface = init_surface(animation_layout -> spritesheet_path);

	const GLsizei frame_size[2] = {
		peek_surface -> w / (GLsizei) animation_layout -> frames_across,
		peek_surface -> h / (GLsizei) animation_layout -> frames_down
	};

	deinit_surface(peek_surface);

	const GLuint albedo_texture_set = init_texture_set(
		true, false, TexNonRepeating,
		OPENGL_LEVEL_MAG_FILTER, OPENGL_LEVEL_MIN_FILTER, 0, 1,
		frame_size[0], frame_size[1], NULL, animation_layout
	);

	////////// Making a world shader, and setting the material index uniform

	const GLuint world_shader = init_shader(
		"shaders/weapon_sprite.vert", NULL,
		"shaders/common/world_shading.frag", NULL
	);

	use_shader(world_shader);
	INIT_UNIFORM_VALUE(weapon_sprite_material_index, world_shader, 1ui, material_index);

	////////// Making a depth prepass shader, and setting the albedo sampler uniform

	const GLuint depth_prepass_shader = init_shader(
		"shaders/weapon_sprite_depth_prepass.vert", NULL,
		"shaders/weapon_sprite_depth_prepass.frag", NULL
	);

	use_shader(depth_prepass_shader);
	use_texture_in_shader(albedo_texture_set, depth_prepass_shader, "albedo_sampler", TexSet, TU_WeaponSpriteAlbedo);

	//////////

	return (WeaponSprite) {
		.drawable = init_drawable_with_vertices(
			define_vertex_spec, (uniform_updater_t) update_uniforms, GL_DYNAMIC_DRAW,
			GL_TRIANGLE_STRIP, (List) {NULL, sizeof(vec3), corners_per_quad, corners_per_quad},

			world_shader, albedo_texture_set,

			init_normal_map_from_albedo_texture(
				normal_map_creator,
				&config -> shared_material_properties.normal_map_config,
				albedo_texture_set, TexSet
			)
		),

		.depth_prepass_shader = depth_prepass_shader,

		.world_space_uniform_ids = {
			INIT_UNIFORM_ID(frame_index, world_shader),
			INIT_UNIFORM_ID(world_corners, world_shader),
			INIT_UNIFORM_ID(tbn, world_shader)
		},

		.depth_prepass_uniform_ids = {
			INIT_UNIFORM_ID(frame_index, depth_prepass_shader),
			INIT_UNIFORM_ID(world_corners, depth_prepass_shader)
		},

		.animation_context = {
			.cycle_start_time = 0.0f,
			.texture_id = 0,
			.activated_in_this_tick = false,
			.is_currently_being_animated = false,

			.animation = {
				.material_index = material_index,
				.texture_id_range = {.start = 0, .end = animation_layout -> total_frames},
				.secs_for_frame = animation_layout -> secs_for_frame
			}
		},

		.appearance_context = {
			.screen_space = {
				.frame_width_over_height = (GLfloat) frame_size[0] / frame_size[1],
				.size = config -> screen_space_size,
				.max_movement_magnitude = config -> max_movement_magnitude,
				.half_movement_cycles_per_sec = 1.0f / (config -> secs_per_movement_cycle * 0.5f)
			},

			.world_space = {
				.max_yaw = glm_rad(config -> max_degrees.yaw),
				.max_pitch = glm_rad(config -> max_degrees.pitch),
				// The world corners are uninitialized here
			}
		},

		// And the OpenAL variables are unitialized as well
		.openal_variables_are_uninitialized = true
	};
}

void deinit_weapon_sprite(const WeaponSprite* const ws) {
	deinit_shader(ws -> depth_prepass_shader);
	deinit_drawable(ws -> drawable);
}

//////////

// The sound emitting pos is in the middle of the top edge of the weapon
static void get_sound_emitting_pos(const vec3 world_corners[corners_per_quad], vec3 sound_emitting_pos) {
	glm_vec3_add((GLfloat*) world_corners[2], (GLfloat*) world_corners[3], sound_emitting_pos);
	glm_vec3_scale(sound_emitting_pos, 0.5f, sound_emitting_pos);
}

void update_weapon_sprite(WeaponSprite* const ws, const Camera* const camera, const Event* const event) {
	const vec3* const world_corners = ws -> appearance_context.world_space.corners;

	if (ws -> openal_variables_are_uninitialized) {
		// This initializes the world corners the first time around
		vec2 screen_corners[corners_per_quad];
		get_screen_corners(ws, event, camera -> speed_xz_percent, screen_corners);
		get_world_corners(screen_corners, camera, &ws -> appearance_context);

		// And this initializes the OpenAL variables
		get_sound_emitting_pos(world_corners, ws -> curr_sound_emitting_pos);
		glm_vec3_zero(ws -> velocity);

		update_weapon_sprite_animation(&ws -> animation_context, event);

		ws -> openal_variables_are_uninitialized = false;
		return;
	}

	////////// Getting the previous sound emitting pos

	vec3 last_sound_emitting_pos;
	get_sound_emitting_pos(world_corners, last_sound_emitting_pos);

	////////// Updating the animation, and then getting new screen and world corners

	update_weapon_sprite_animation(&ws -> animation_context, event);

	vec2 screen_corners[corners_per_quad];
	get_screen_corners(ws, event, camera -> speed_xz_percent, screen_corners);
	get_world_corners(screen_corners, camera, &ws -> appearance_context);

	////////// Getting the current sound emitting pos, and the velocity from that

	GLfloat* const curr_sound_emitting_pos = ws -> curr_sound_emitting_pos, *const velocity = ws -> velocity;
	get_sound_emitting_pos(world_corners, curr_sound_emitting_pos);

	glm_vec3_sub(curr_sound_emitting_pos, last_sound_emitting_pos, velocity);
	glm_vec3_scale(velocity, 1.0f / event -> delta_time, velocity);
}

void draw_weapon_sprite_to_shadow_context(const WeaponSprite* const ws) {
	const Drawable* const drawable = &ws -> drawable;

	use_vertex_buffer(drawable -> vertex_buffer);
	reinit_vertex_buffer_data(corners_per_quad, sizeof(vec3), ws -> appearance_context.world_space.corners);
	draw_drawable(*drawable, corners_per_quad, 0, NULL, UseVertexSpec);
}

void draw_weapon_sprite_for_depth_prepass(const WeaponSprite* const ws) {
	// TODO: share logic between this depth prepass and the one for sectors (redundant state changes otherwise).

	const Drawable* const drawable = &ws -> drawable;
	const GLuint shader = ws -> depth_prepass_shader;
	use_shader(shader);

	// Allowing all fragments to pass (except for discarded ones), since it's drawn in front of everything else
	WITH_RENDER_STATE(glDepthFunc, GL_ALWAYS, constants.default_depth_func,
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
			UPDATE_UNIFORM(ws, depth_prepass, frame_index, 1ui, ws -> animation_context.texture_id);
			UPDATE_UNIFORM(ws, depth_prepass, world_corners, 3fv, corners_per_quad, (GLfloat*) ws -> appearance_context.world_space.corners);
			draw_primitives(drawable -> triangle_mode, corners_per_quad);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	);
}

// TODO: can I stop shading early for the transparent part of the weapon sprite, without `discard`?
void draw_weapon_sprite(const WeaponSprite* const ws) {
	/* Not using `GL_EQUAL` after the weapon-sprite depth prepass, because
	only depth buffer values that correspond with an alpha value of 1
	are written to the depth buffer. Otherwise, proper alpha blending would not happen. */

	draw_drawable(ws -> drawable, corners_per_quad, 0, &(UniformUpdaterParams) {ws}, UseShaderPipeline);
}

////////// Sound functions

bool weapon_sound_activator(const void* const data) {
	/* TODO: maybe put the `activated_in_this_tick` variable in the `Animation`
	struct (if that's needed for other non-weapon-sprite objects) */
	return ((WeaponSprite*) data) -> animation_context.activated_in_this_tick;
}

void weapon_sound_updater(const void* const data, const ALuint al_source) {
	const WeaponSprite* const ws = data;

	const vec3* const world_corners = ws -> appearance_context.world_space.corners;
	const GLfloat *const bl = world_corners[0], *const tl = world_corners[2];

	vec3 dir; // The dir on the left will be the same as the dir on the right
	glm_vec3_sub((GLfloat*) tl, (GLfloat*) bl, dir);
	glm_vec3_normalize(dir);

	alSourcefv(al_source, AL_POSITION, ws -> curr_sound_emitting_pos);
	alSourcefv(al_source, AL_VELOCITY, ws -> velocity);
	alSourcefv(al_source, AL_DIRECTION, dir);
}
