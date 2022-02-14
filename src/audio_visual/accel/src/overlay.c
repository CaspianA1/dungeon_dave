/*
- Overlay = weapon, HUD, and things like that

Weapon sprite TODO:
- Put weapon shaders in shaders.c
- Configure mag and min filter as a variable
- Structure weapon as an animation
- Draw weapon as actual 3D object - it should jut outwards
- Rescale the weapon to make it not warped by the screen resolution
*/

#include "headers/overlay.h"
#include "headers/constants.h"
#include "texture.c"
#include "utils.c"

const GLchar *const weapon_sprite_vertex_shader =
	"#version 330 core\n"

	"uniform float frame_width_over_height, weapon_size_screen_space;\n"
	"uniform vec2 pace;\n"

	"out vec2 fragment_UV;\n"

	"const vec2 corners[4] = vec2[4] (\n"
		"vec2(-1.0f, -1.0f), vec2(1.0f, -1.0f),\n"
		"vec2(-1.0f, 1.0f), vec2(1.0f, 1.0f)\n"
	");\n"

	"void main(void) {\n"
		"vec2 corner = corners[gl_VertexID];\n"

		"vec2 on_screen_corner = corner * weapon_size_screen_space;\n"
		"on_screen_corner.x *= frame_width_over_height;\n"
		"on_screen_corner.y += weapon_size_screen_space - 1.0f;\n" // Makes weapon touch bottom of screen

		"on_screen_corner += pace;\n"

		"gl_Position = vec4(on_screen_corner, 0.2f, 1.0f);\n"
		"fragment_UV = vec2(corner.x, -corner.y) * 0.5f + 0.5f;\n"
	"}\n",

*const weapon_sprite_fragment_shader =
	"#version 330 core\n"

	"in vec2 fragment_UV;\n"

	"out vec4 color;\n"

	"uniform uint frame_index;\n"
	"uniform sampler2DArray frame_sampler;\n"

	"void main(void) {\n"
		"color = texture(frame_sampler, vec3(fragment_UV, frame_index));\n"
	"}\n";

WeaponSprite init_weapon_sprite(const GLfloat size, const GLchar* const spritesheet_path,
	const GLsizei frames_across, const GLsizei frames_down, const GLsizei total_frames) {

	/* It's a bit wasteful to load the surface in init_texture_set
	and here, but this makes the code much more readable */

	SDL_Surface* const peek_surface = init_surface(spritesheet_path);
	const GLsizei frame_size[2] = {peek_surface -> w / frames_across, peek_surface -> h / frames_down};
	deinit_surface(peek_surface);

	return (WeaponSprite) {
		.texture = init_texture_set(TexNonRepeating, 0, 1, frame_size[0],
			frame_size[1], spritesheet_path, frames_across, frames_down, total_frames),

		.shader = init_shader_program(weapon_sprite_vertex_shader, weapon_sprite_fragment_shader),
		.curr_frame = 0,
		.frame_width_over_height = (GLfloat) frame_size[0] / frame_size[1],
		.size = size
	};
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

void draw_weapon_sprite(const WeaponSprite ws, const Camera* const camera) {
	glUseProgram(ws.shader);

	static byte first_call = 1;
	static GLint weapon_size_screen_space_id, frame_index_id, pace_id;

	// TODO: put these in constants.c
	const GLfloat max_movement_magitude = 0.2f, time_for_half_movement_cycle = 0.8f;

	if (first_call) {
		INIT_UNIFORM_VALUE(frame_width_over_height, ws.shader, 1f, ws.frame_width_over_height);
		INIT_UNIFORM_VALUE(weapon_size_screen_space, ws.shader, 1f, ws.size);

		INIT_UNIFORM(weapon_size_screen_space, ws.shader);
		INIT_UNIFORM(frame_index, ws.shader);
		INIT_UNIFORM(pace, ws.shader);

		use_texture(ws.texture, ws.shader, "frame_sampler", TexSet, WEAPON_TEXTURE_UNIT);

		first_call = 0;
	}

	const GLfloat
		curr_time = SDL_GetTicks() / 1000.0f,
		smooth_speed_xz_percent = circular_mapping_from_zero_to_one(camera -> speed_xz_percent);

	const GLfloat
		time_pace = sinf(curr_time * PI / time_for_half_movement_cycle),
		weapon_movement_magnitude = max_movement_magitude * smooth_speed_xz_percent;

	const GLfloat across = time_pace * weapon_movement_magnitude * smooth_speed_xz_percent; // From -magnitude to magnitude
	const GLfloat down = (fabsf(across) - weapon_movement_magnitude) * smooth_speed_xz_percent; // From 0.0f to -magnitude

	UPDATE_UNIFORM(pace, 2f, across, down);
	UPDATE_UNIFORM(frame_index, 1ui, (keys[SDL_SCANCODE_T] + keys[SDL_SCANCODE_Y] + keys[SDL_SCANCODE_U] * 2) * 3);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDisable(GL_BLEND);
}

void deinit_weapon_sprite(const WeaponSprite ws) {
	deinit_texture(ws.texture);
	glDeleteProgram(ws.shader);
}
