#ifndef OVERLAY_C
#define OVERLAY_C

/*
- Overlay = weapon, HUD, and things like that

Weapon sprite TODO:
- Put weapon shaders in shaders.c
- Configure mag and min filter as a variable

- Stop the weapon from clipping with walls and things
- Align the bottom of the weapon with the bottom of the screen
- Angle it when looking up and down
*/

#include "headers/overlay.h"
#include "headers/constants.h"
#include "utils.c"
#include "texture.c"
#include "animation.c"
#include "skybox.c"

const GLchar *const weapon_sprite_vertex_shader =
	"#version 330 core\n"

	"out vec2 fragment_UV;\n"
	"out vec3 vertex_world_space;\n"

	"uniform float size, frame_width_over_height;\n"
	"uniform vec2 pace;\n"
	"uniform vec3 camera_pos, camera_dir, camera_right, camera_up;\n"
	"uniform mat4 model_view_projection;\n"

	"const float dist_from_camera = 0.4f;\n"

	"const vec2 corners_model_space[4] = vec2[4] (\n"
		"vec2(-0.5f, -0.5f), vec2(0.5f, -0.5f),\n"
		"vec2(-0.5f, 0.5f), vec2(0.5f, 0.5f)\n"
	");\n"

	"void main(void) {\n"
		"vec2 vertex_model_space = corners_model_space[gl_VertexID];\n"
		"vec3 center_world_space = camera_pos + camera_dir * dist_from_camera;\n"

		// "if (gl_VertexID <= 1) center_world_space.xz -= camera_dir.xz * 0.3f;\n"

		"vertex_world_space = center_world_space\n"
			"+ vertex_model_space.x * camera_right * size"
			"+ vertex_model_space.y * camera_up * size;\n"

		"gl_Position = model_view_projection * vec4(vertex_world_space, 1.0f);\n"
		"gl_Position.x *= frame_width_over_height;\n"
		"gl_Position.xy += pace;\n"

		"fragment_UV = vec2(vertex_model_space.x, -vertex_model_space.y) + 0.5f;\n"
	"}\n",

*const weapon_sprite_fragment_shader =
	"#version 330 core\n"

	"in vec2 fragment_UV;\n"
	"in vec3 vertex_world_space;\n"

	"out vec4 color;\n"

	"uniform uint frame_index;\n"
	"uniform vec3 camera_pos, camera_dir;\n"
	"uniform sampler2DArray frame_sampler;\n"
	"uniform samplerCube skybox_sampler;\n"

	"const float skybox_reflection_weight = 0.1f;"

	"void main(void) {\n"
		"vec3 incidence = normalize(vertex_world_space - camera_pos);\n"
		"vec3 reflection = reflect(incidence, -camera_dir);\n"

		"vec3 skybox_color = texture(skybox_sampler, reflection).rgb;\n"
		"vec4 weapon_color = texture(frame_sampler, vec3(fragment_UV, frame_index));\n"
		"color = vec4(mix(weapon_color.rgb, skybox_color, skybox_reflection_weight), weapon_color.a);\n"
	"}\n";

WeaponSprite init_weapon_sprite(const GLfloat size, const GLfloat secs_per_frame,
	const GLchar* const spritesheet_path, const GLsizei frames_across,
	const GLsizei frames_down, const GLsizei total_frames) {

	/* It's a bit wasteful to load the surface in init_texture_set
	and here, but this makes the code much more readable */

	SDL_Surface* const peek_surface = init_surface(spritesheet_path);
	const GLsizei frame_size[2] = {peek_surface -> w / frames_across, peek_surface -> h / frames_down};
	deinit_surface(peek_surface);

	return (WeaponSprite) {
		.texture = init_texture_set(TexNonRepeating, 0, 1, frame_size[0],
			frame_size[1], spritesheet_path, frames_across, frames_down, total_frames),

		.shader = init_shader_program(weapon_sprite_vertex_shader, weapon_sprite_fragment_shader),

		.animation = {.texture_id_range = {.start = 0, .end = total_frames}, .secs_per_frame = secs_per_frame},
		.curr_frame = 0, .last_frame_time = SDL_GetTicks() / 1000.0f,

		.frame_width_over_height = (GLfloat) frame_size[0] / frame_size[1],
		.size = size
	};
}

void deinit_weapon_sprite(const WeaponSprite* const ws) {
	deinit_texture(ws -> texture);
	glDeleteProgram(ws -> shader);
}

// Given an input between 0 and 1, this returns the y-value of the top left side of a circle
static GLfloat circular_mapping_from_zero_to_one(const GLfloat x) {
	/*
	- Goal: a smooth mapping from 0 to 1
	- And the y-value of a circle from x = 0 to 1, when its center equals (1, 0), achieves just that

	Circle equation: x^2 + y^2 = 1
	Shifted 1 unit to the right: (x - 1)^2 + y^2 = 1
	y^2 = 1 - (x - 1)^2
	y = sqrt(1 - (x - 1)^2) */

	const GLfloat x_minus_one = x - 1.0f;
	return sqrtf(1.0f - x_minus_one * x_minus_one);
}

static void update_weapon_sprite(WeaponSprite* const ws, const Event* const event) {
	buffer_size_t curr_frame = ws -> curr_frame;

	if (curr_frame == 0) {
		if ((event -> movement_bits & BIT_USE_WEAPON) != 0) curr_frame++;
	}
	else {
		update_animation_information(
			&ws -> last_frame_time, &curr_frame,
			ws -> animation, SDL_GetTicks() / 1000.0f);
	}

	ws -> curr_frame = curr_frame;
}

void update_and_draw_weapon_sprite(WeaponSprite* const ws_ref, const Camera* const camera,
	const Event* const event, const Skybox* const skybox) {

	update_weapon_sprite(ws_ref, event);
	const WeaponSprite ws = *ws_ref;

	glUseProgram(ws.shader);

	static byte first_call = 1;

	static GLint pace_id, size_id,
		frame_index_id, camera_pos_id,
		camera_dir_id, camera_right_id,
		camera_up_id, model_view_projection_id;

	// TODO: put these in constants.c
	const GLfloat max_movement_magitude = 0.2f, time_for_half_movement_cycle = 0.5f;

	if (first_call) {
		INIT_UNIFORM_VALUE(size, ws.shader, 1f, ws.size);
		INIT_UNIFORM_VALUE(frame_width_over_height, ws.shader, 1f, ws.frame_width_over_height);

		INIT_UNIFORM(pace, ws.shader);
		INIT_UNIFORM(size, ws.shader);
		INIT_UNIFORM(frame_index, ws.shader);
		INIT_UNIFORM(camera_pos, ws.shader);
		INIT_UNIFORM(camera_dir, ws.shader);
		INIT_UNIFORM(camera_right, ws.shader);
		INIT_UNIFORM(camera_up, ws.shader);
		INIT_UNIFORM(model_view_projection, ws.shader);

		use_texture(ws.texture, ws.shader, "frame_sampler", TexSet, WEAPON_TEXTURE_UNIT);
		use_texture(skybox -> texture, ws.shader, "skybox_sampler", TexSkybox, SKYBOX_TEXTURE_UNIT);

		first_call = 0;
	}

	const GLfloat
		curr_time = SDL_GetTicks() / 1000.0f,
		smooth_speed_xz_percent = circular_mapping_from_zero_to_one(camera -> speed_xz_percent);

	const GLfloat
		time_pace = sinf(curr_time * PI / time_for_half_movement_cycle),
		weapon_movement_magnitude = max_movement_magitude * smooth_speed_xz_percent;

	const GLfloat across = time_pace * weapon_movement_magnitude * 0.5f * smooth_speed_xz_percent; // From -magnitude / 2 to magnitude / 2
	const GLfloat down = (fabsf(across) - weapon_movement_magnitude) * smooth_speed_xz_percent; // From 0.0f to -magnitude

	UPDATE_UNIFORM(pace, 2f, across, down);
	UPDATE_UNIFORM(frame_index, 1ui, ws.curr_frame);

	UPDATE_UNIFORM(camera_pos, 3fv, 1, camera -> pos);
	UPDATE_UNIFORM(camera_dir, 3fv, 1, camera -> dir);
	UPDATE_UNIFORM(camera_right, 3fv, 1, camera -> right);
	UPDATE_UNIFORM(camera_up, 3fv, 1, camera -> up);

	UPDATE_UNIFORM(model_view_projection, Matrix4fv, 1, GL_FALSE, &camera -> model_view_projection[0][0]);

	// glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisable(GL_BLEND);
	// glEnable(GL_DEPTH_TEST);
}

#endif
