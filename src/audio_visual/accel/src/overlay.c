/*
- Overlay = weapon, HUD, and things like that

Weapon TODO:
- Put weapon shaders in shaders.c
- Configure mag and min filter as a variable
- Structure weapon as an animation
- Blending
- Draw weapon as actual 3D object - it should jut outwards
- Make pace while jumping smoother
- Make weapon pacing less violent
- Center weapon pace at center of screen
*/

#include "headers/overlay.h"
#include "headers/constants.h"
#include "texture.c"
#include "utils.c"

const GLchar *const weapon_vertex_shader =
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

*const weapon_fragment_shader =
	"#version 330 core\n"

	"in vec2 fragment_UV;\n"

	"out vec4 color;\n"

	"uniform uint frame_index;\n"
	"uniform sampler2DArray frame_sampler;\n"

	"void main(void) {\n"
		"color = texture(frame_sampler, vec3(fragment_UV, frame_index));\n"
	"}\n";

Weapon init_weapon(const GLchar* const spritesheet_path, const GLsizei frames_across,
	const GLsizei frames_down, const GLsizei total_frames) {

	/* It's a bit wasteful to load the surface in init_texture_set
	and here, but this makes the code much more readable */

	SDL_Surface* const peek_surface = init_surface(spritesheet_path);
	const GLsizei frame_size[2] = {peek_surface -> w / frames_across, peek_surface -> h / frames_down};
	deinit_surface(peek_surface);

	const Weapon weapon = {
		.texture = init_texture_set(TexNonRepeating, 0, 1, frame_size[0],
			frame_size[1], spritesheet_path, frames_across, frames_down, total_frames
		),

		.shader = init_shader_program(weapon_vertex_shader, weapon_fragment_shader),
		.frame_width_over_height = (GLfloat) frame_size[0] / frame_size[1],
		.curr_frame = 0
	};

	return weapon;
}

void draw_weapon(const Weapon weapon, const Camera* const camera) {
	glUseProgram(weapon.shader);

	static byte first_call = 1;
	static GLint weapon_size_screen_space_id, frame_index_id, pace_id;

	// TODO: put these in constants.c
	const GLfloat weapon_size = 0.45f, weapon_movement_magitude = 0.2f, time_for_half_weapon_movement_cycle = 0.8f;

	if (first_call) {
		INIT_UNIFORM_VALUE(frame_width_over_height, weapon.shader, 1f, weapon.frame_width_over_height);
		INIT_UNIFORM_VALUE(weapon_size_screen_space, weapon.shader, 1f, weapon_size);

		INIT_UNIFORM(weapon_size_screen_space, weapon.shader);
		INIT_UNIFORM(frame_index, weapon.shader);
		INIT_UNIFORM(pace, weapon.shader);

		use_texture(weapon.texture, weapon.shader, "frame_sampler", TexSet, WEAPON_TEXTURE_UNIT);

		first_call = 0;
	}

	const GLfloat curr_time = SDL_GetTicks() / 1000.0f;

	const GLfloat
		time_pace = sinf(curr_time * PI / time_for_half_weapon_movement_cycle),
		smooth_speed_xz_percent = log2f(camera -> speed_xz_percent + 1.0f);

	const GLfloat across = time_pace * smooth_speed_xz_percent * weapon_movement_magitude; // From -magnitude to magnitude
	const GLfloat down = (fabsf(across) - weapon_movement_magitude) * smooth_speed_xz_percent; // From 0.0f to -magnitude

	UPDATE_UNIFORM(pace, 2f, across, down);
	UPDATE_UNIFORM(frame_index, 1ui, keys[SDL_SCANCODE_T] + keys[SDL_SCANCODE_Y]);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDisable(GL_BLEND);
}

void deinit_weapon(const Weapon weapon) {
	deinit_texture(weapon.texture);
	glDeleteProgram(weapon.shader);
}
