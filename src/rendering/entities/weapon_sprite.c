#include "rendering/entities/weapon_sprite.h"
#include "data/constants.h"
#include "utils/texture.h"
#include "utils/shader.h"
#include "utils/opengl_wrappers.h"

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
	Inside that function, it is drawn through `draw_drawable_to_shadow_context`.
	A function is injected into that, which copies over its world-space corners on the CPU
	to its vertex buffer which holds the vertices on the GPU. It is then drawn.
	TODO: let alpha values in the weapon sprite's texture determine its visiblity.

3. Finally, it is drawn to the default framebuffer in `draw_weapon_sprite`. No vertex buffer
or spec are used for this; the four corners are simply passed in as a uniform. I figured that
there wouldn't be much point of binding a vertex buffer and spec if the vertex count is known
ahead of time, and is very small; so this should make the code a bit simpler and marginally faster. */

/* TODO: fix this:
- When debugging and separating the weapon sprite from the position of the player (by not updating the corners), these two things can happen:
	1. If the near clip dist is way too large, the weapon grows in size a ton.
	2. If the near clip dist is way too small, the weapon appears warped, and disappears from sight too easily.
*/

//////////

static void update_weapon_sprite_animation(WeaponSpriteAnimationContext* const animation_context, const Event* const event) {
	buffer_size_t curr_frame = animation_context -> curr_frame;
	const GLfloat curr_time_secs = event -> curr_time_secs;

	if (curr_frame == 0) {
		if (CHECK_BITMASK(event -> movement_bits, BIT_USE_WEAPON)) {
			animation_context -> cycle_base_time = curr_time_secs;
			curr_frame++;
		}
	}
	else update_animation_information(
		curr_time_secs,
		animation_context -> cycle_base_time,
		animation_context -> animation, &curr_frame
	);

	animation_context -> curr_frame = curr_frame;
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
	glm_vec3_normalize(tangent); // Flows along S, from bl to br

	glm_vec3_sub((GLfloat*) bl_corner, (GLfloat*) quad_corners[2], bitangent);
	glm_vec3_normalize(bitangent); // Flows along T, from bl to tl

	// This will also be normalized, as the tangent and bitangent are normalized
	glm_vec3_cross(tangent, bitangent, normal);
}

////////// This part is for the uniform updater param type and the uniform updater

typedef struct {const WeaponSprite* const weapon_sprite;} UniformUpdaterParams;

static void update_uniforms(const Drawable* const drawable, const void* const param) {
	const UniformUpdaterParams typed_params = *(UniformUpdaterParams*) param;

	static GLint frame_index_id, world_corners_id, tbn_id;

	ON_FIRST_CALL(
		const GLuint shader = drawable -> shader;

		INIT_UNIFORM(frame_index, shader);
		INIT_UNIFORM(world_corners, shader);
		INIT_UNIFORM(tbn, shader);
	);

	////////// Updating uniforms

	const vec3* const world_corners = typed_params.weapon_sprite -> appearance_context.world_space.corners;

	mat3 tbn;
	get_quad_tbn_matrix(world_corners, tbn);

	UPDATE_UNIFORM(frame_index, 1ui, typed_params.weapon_sprite -> animation_context.curr_frame);
	UPDATE_UNIFORM(world_corners, 3fv, corners_per_quad, (GLfloat*) world_corners);
	UPDATE_UNIFORM(tbn, Matrix3fv, 1, GL_FALSE, (GLfloat*) tbn);
}

////////// Initialization, deinitialization, updating, and rendering

static void define_vertex_spec(void) {
	define_vertex_spec_index(false, true, 0, 3, 0, 0, GL_FLOAT);
}

WeaponSprite init_weapon_sprite(
	const GLfloat max_yaw_degrees, const GLfloat max_pitch_degrees,
	const GLfloat screen_space_size, const GLfloat secs_per_frame,
	const GLfloat secs_per_movement_cycle,
	const GLfloat max_movement_magnitude,
	const AnimationLayout* const animation_layout,
	const NormalMapConfig* const normal_map_config) {

	////////// Getting the frame size and a diffuse texture set

	/* It's a bit wasteful to load the surface in `init_texture_set`
	and here too, but this makes the code much more readable. */
	SDL_Surface* const peek_surface = init_surface(animation_layout -> spritesheet_path);

	const GLsizei frame_size[2] = {
		peek_surface -> w / animation_layout -> frames_across,
		peek_surface -> h / animation_layout -> frames_down
	};

	deinit_surface(peek_surface);

	const GLuint diffuse_texture_set = init_texture_set(
		true, TexNonRepeating,
		OPENGL_SCENE_MAG_FILTER, OPENGL_SCENE_MIN_FILTER, 0, 1,
		frame_size[0], frame_size[1], NULL, animation_layout
	);

	//////////

	return (WeaponSprite) {
		.drawable = init_drawable_with_vertices(
			define_vertex_spec, (uniform_updater_t) update_uniforms, GL_DYNAMIC_DRAW,
			GL_TRIANGLE_STRIP, (List) {NULL, sizeof(vec3), corners_per_quad, corners_per_quad},

			init_shader(ASSET_PATH("shaders/weapon_sprite.vert"), NULL, ASSET_PATH("shaders/weapon_sprite.frag"), NULL),
			diffuse_texture_set, init_normal_map_from_diffuse_texture(diffuse_texture_set, TexSet, normal_map_config)
		),

		.animation_context = {
			.cycle_base_time = 0.0f, .curr_frame = 0,
			.animation = {
				.texture_id_range = {.start = 0, .end = (buffer_size_t) animation_layout -> total_frames},
				.secs_for_frame = secs_per_frame
			}
		},

		.appearance_context = {
			.screen_space = {
				.frame_width_over_height = (GLfloat) frame_size[0] / frame_size[1],
				.size = screen_space_size,
				.max_movement_magnitude = max_movement_magnitude,
				.half_movement_cycles_per_sec = 1.0f / (secs_per_movement_cycle * 0.5f)
			},

			.world_space = {.max_yaw = glm_rad(max_yaw_degrees), .max_pitch = glm_rad(max_pitch_degrees)}
		}
	};
}

void update_weapon_sprite(WeaponSprite* const ws, const Camera* const camera, const Event* const event) {
	update_weapon_sprite_animation(&ws -> animation_context, event);

	WeaponSpriteAppearanceContext* const appearance_context = &ws -> appearance_context;

	vec2 screen_corners[corners_per_quad];
	get_screen_corners(ws, event, camera -> speed_xz_percent, screen_corners);
	get_world_corners(screen_corners, camera, appearance_context);
}

void draw_weapon_sprite_to_shadow_context(const WeaponSprite* const ws) {
	const Drawable* const drawable = &ws -> drawable;

	use_vertex_buffer(drawable -> vertex_buffer);
	reinit_vertex_buffer_data(corners_per_quad, sizeof(vec3), ws -> appearance_context.world_space.corners);
	draw_drawable(*drawable, corners_per_quad, 0, NULL, BindVertexSpec);
}

void draw_weapon_sprite(const WeaponSprite* const ws) {
	// No depth testing b/c depth values from sectors or billboards may intersect
	WITH_RENDER_STATE(glDepthFunc, GL_ALWAYS, GL_LESS,
		draw_drawable(ws -> drawable, corners_per_quad, 0,
			&(UniformUpdaterParams) {ws}, UseShaderPipeline
		);
	);
}
