// Overlay = weapon, HUD, and things like that

#include "headers/overlay.h"
#include "texture.c"
#include "utils.c"

// TODO: put these in shaders.c later
const GLchar *const weapon_vertex_shader =
	"#version 330 core\n"
	
	"layout(location = 0) in vec2 vertex_pos_screen_space;\n"
	"layout(location = 1) in vec2 UV;\n"

	"out vec2 fragment_UV;\n"

	"void main(void) {\n"
		"gl_Position = vec4(vertex_pos_screen_space, 0.0f, 1.0f);\n"
		"fragment_UV = UV;\n"
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
	const GLsizei peek_size[2] = {peek_surface -> w, peek_surface -> h};
	deinit_surface(peek_surface);

	const Weapon weapon = {
		.texture = init_texture_set(
			TexNonRepeating, 0, 1, peek_size[0], peek_size[1],

			spritesheet_path, frames_across, frames_down, total_frames
		),

		.shader = init_shader_program(weapon_vertex_shader, weapon_fragment_shader),

		.curr_frame = 0
	};

	static byte first_call = 1;
	if (first_call) {
		glUseProgram(weapon.shader);
		use_texture(weapon.texture, weapon.shader, "frame_sampler", TexSet, WEAPON_TEXTURE_UNIT);
		first_call = 0;
	}

	return weapon;
}

// Make sure that blending will work

void deinit_weapon(const Weapon weapon) {
	deinit_texture(weapon.texture);
	glDeleteProgram(weapon.shader);
}
